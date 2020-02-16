#include <iostream>
#include "server.h"

Server::Server() {
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // 初始化socket
    listener = 0;

    epfd = 0;
}

// 初始化服务并启动监听
void Server::Init() {
    std::cout << "Init Server..." << std::endl;

    // 创建监听socket
    listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) { 
        perror("listener error");
        exit(-1);
    }
    
    // 绑定地址
    if (bind(listener, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("bind error");
        exit(-1);
    }

    // 监听
    int ret = listen(listener, 5);
    if (ret < 0) {
        perror("listen error");
        exit(-1);
    }

    std::cout << "Start to listen: " << SERVER_IP << std::endl;

    // 在内核中创建事件表 epfd是一个句柄
    epfd = epoll_create(EPOLL_SIZE);
    if (epfd < 0) {
        perror("epfd error");
        exit(-1);
    }
    // 往事件表里面添加监听时间
    addfd(epfd, listener, true);
}

// 关闭服务, 清理并关闭描述文件
void Server::Close() {
    close(listener);
    close(epfd);
}

// 发送广告消息给所有客户端
int Server::SendBroadcastMessage(int clientfd) {
    char recv_buf[BUF_SIZE];
    char send_buf[BUF_SIZE];
    Msg msg;
    bzero(recv_buf, BUF_SIZE);
    
    // 接收新消息
    std::cout << "read from client(clientID = " << clientfd << ")" << std::endl;
    int len = recv(clientfd, recv_buf, BUF_SIZE, 0);
    // 清空结构体, 把接受到的字符串转化为结构体
    memset(&msg, 0, sizeof(msg));
    memcpy(&msg, recv_buf, sizeof(msg));
    // 判断接受的信息是私聊还是群聊
    msg.fromID = clientfd;
    if (msg.content[0] == '\\' && isdigit(msg.content[1])) {
        msg.type = 1;    
        msg.toID = msg.content[1] - '0';
        memcpy(msg.content, msg.content + 2, sizeof(msg.content));
    } else {
        msg.type = 0;
    }
    // 若recv返回值为0, 说明该客户端有问题
    if (len == 0) {
        close(clientfd);    
        // 在客户端列表中删除该客户端
        clients_list.remove(clientfd);
        std::cout << "ClientID = " << clientfd
            << " closed.\n now there are " 
            << clients_list.size()
            << "clients in the chat room" 
            << std::endl;
    } else {
        if (clients_list.size() == 1) {
            // 发送提示消息
            memcpy(&msg.content, CAUTION, sizeof(msg.content));
            bzero(send_buf, BUF_SIZE);
            memcpy(send_buf, &msg, sizeof(msg));
            send(clientfd, send_buf, sizeof(send_buf), 0);
            return len;
        }
    }
    // 存放格式化后的信息
    char format_message[BUF_SIZE];
    // 群聊
    if (msg.type == 0) {
        // 格式化发送的消息内容SERVER_MESSAGE 
        sprintf(format_message, SERVER_MESSAGE, clientfd, msg.content);
        memcpy(msg.content, format_message, BUF_SIZE);
        std::list<int>::iterator it = clients_list.begin();
        for (; it != clients_list.end(); ++it) {
            if(*it != clientfd) {
                // 发送的结构体转化为字符串
                bzero(send_buf, BUF_SIZE);
                memcpy(send_buf, &msg, sizeof(msg));
                if (send(*it, send_buf, sizeof(send_buf), 0) < 0) {
                    return -1;
                }
            } 
        }
    }
    if (msg.type == 1) {
        bool private_offline = true;
        sprintf(format_message, SERVER_PRIVATE_MESSAGE, clientfd, msg.content);
        memcpy(msg.content, format_message, BUF_SIZE);
        std::list<int>::iterator it = clients_list.begin();
        for (; it != clients_list.end(); ++it) {
            if (*it != msg.toID) {
                continue; 
            }    
            private_offline = false;
            bzero(send_buf, BUF_SIZE);
            memcpy(send_buf, &msg, sizeof(msg));
            if (send(*it, send_buf, sizeof(send_buf), 0) < 0) {
                return -1;
            }
        }
        if (private_offline) {
            sprintf(format_message, SERVER_PRIVATE_ERROR_MESSAGE, msg.toID);    
            memcpy(msg.content, format_message, BUF_SIZE); 
            bzero(send_buf, BUF_SIZE);
            memcpy(send_buf, &msg, sizeof(msg));
            if (send(msg.fromID, send_buf, sizeof(send_buf), 0) < 0) {
                return -1;
            }
        }
    }
    return len;
}


//启动服务端
void Server::Start() {
    // epoll 事件队列
    static struct epoll_event events[EPOLL_SIZE];
    Init();
    
    while(true) {
        // epoll_events_count表示就绪事件数目
        // 会取出所有活跃的有反馈的事件, 事件集合为{新用户连接, 已连接用户消息传递}
        int epoll_events_count = epoll_wait(epfd, events, EPOLL_SIZE, -1);
        if (epoll_events_count < 0) {
            perror("epoll failure"); 
            break;
        }
        std::cout << "epoll_events_count =\n" << epoll_events_count << std::endl;
        for (int i = 0; i < epoll_events_count; ++i) {
            int sockfd = events[i].data.fd;    
            // 若事件是新用户连接, 做如下处理
            if (sockfd == listener) {
                struct sockaddr_in client_address;   
                socklen_t client_addrLength = sizeof(struct sockaddr_in); 
                int clientfd = accept(listener, (struct sockaddr*) &client_address, &client_addrLength);
                std::cout << "client connection from: "
                    <<inet_ntoa(client_address.sin_addr) << ":"
                    << ntohs(client_address.sin_port) << ", clientfd = "
                    << clientfd << std::endl;
                addfd(epfd, clientfd, true);
                // clientfd 存储在clients_list中
                clients_list.push_back(clientfd);
                std::cout << "Add new clientfd = " << clientfd << " to epoll" << std::endl;
                std::cout << "Now there are " << clients_list.size() 
                    << " clients int the chat room" << std::endl;

                // 欢迎信息
                std::cout << "welcome message" << std::endl;
                char message[BUF_SIZE];
                bzero(message, BUF_SIZE);
                sprintf(message, SERVER_WELCOME, clientfd);
                if (send(clientfd, message, sizeof(message), 0) < 0) {
                    perror("send error");    
                    Close();
                    exit(-1);
                }
            } 
            // 否则事件为连接用户发送消息
            else {
                int ret = SendBroadcastMessage(sockfd);
                if (ret < 0) {
                    perror("error");    
                    Close();
                    exit(-1);
                }
            }
        }
    }
    // 关闭服务
    Close();
}


