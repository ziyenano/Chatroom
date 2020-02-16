#include <iostream>
#include "client.h"

Client::Client() {
    // 初始化要链接的服务器和端口
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    // 初始化socket
    sock = 0;
    // 初始化进程号
    pid = 0;
    // 客户端状态
    isClinetwork = true;
    // epool fd
    epfd = 0;
}

// 连接服务器
void Client::Connect() {
    std::cout << "Connect Server: " << SERVER_IP << ":" << SERVER_PORT << std::endl;

    // 创建socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("sock error");
        exit(-1);
    }

    // 连接服务端
    if (connect(sock, (struct sockaddr*) &serverAddr, sizeof(serverAddr)) < 0) {
        perror("connect error");        
        exit(-1);
    }

    // 创建管道, fd[0]用于父进程, fd[1]用于子进程
    if (pipe(pipe_fd) < 0) {
        perror("pipe error");
        exit(-1);
    }

    // 创建epoll
    epfd = epoll_create(EPOLL_SIZE);
    if (epfd < 0) {
        perror("epoll error");
        exit(-1);
    }
    // 将sock和管道读端描述符都添加到内核事件表中
    addfd(epfd, sock, true);
    addfd(epfd, pipe_fd[0], true);
}

void Client::Close() {
    if (pid) {
        // 关闭父进程的管道和sock
        close(pipe_fd[0]);
        close(sock);
    } else {
        close(pipe_fd[1]);
    }
}

// 启动客户端
void Client::Start() {
    // epoll 事件队列
    static struct epoll_event events[2];
    // 连接服务器
    Connect();
    // 创建子进程
    pid = fork();
    // 如果子进程创建失败
    // fork函数返回值, 父进程返回>0, 子进程返回=0
    if (pid < 0) {
        perror("fork error");
        close(sock);   
        exit(-1);
    } else if (pid == 0) {
        // 进入子进程执行流程
        // 子进程负责写入管道, 因此先关闭读入
        close(pipe_fd[0]);

        // 输入exit可以退出聊天
        std::cout << "Please input 'exit' to exit the chat room" << std::endl;
        std::cout << "\\ + ClientID to private chat " << std::endl;
        // 如果客户端正常, 则不断读取输入发送给服务端
        while (isClinetwork) {
            // 清空结构体
            memset(msg.content, 0, sizeof(msg.content));
            fgets(msg.content, BUF_SIZE, stdin);
            // 如果输出exit, 退出
            if (strncasecmp(msg.content, EXIT, strlen(EXIT)) == 0) {
                isClinetwork = 0;    
            } else { 
                // 子进程写入管道
                memset(send_buf, 0, sizeof(BUF_SIZE));
                memcpy(send_buf, &msg, sizeof(msg));
                if (write(pipe_fd[1], send_buf, sizeof(send_buf)) < 0) {
                   perror("fork error"); 
                   exit(-1);
                }
            }
        }
    } else {
        // pid > 0 父进程
        // 父进程负责读取管道, 先关闭写端
        close(pipe_fd[1]);
        while (isClinetwork) {
            int epoll_events_count = epoll_wait(epfd, events, 2, -1);
            // 处理就绪事件, 事件集合为{服务器传来消息, 写入进程有数据}
            for (int i = 0; i < epoll_events_count; ++i) {
               memset(recv_buf, 0 , sizeof(recv_buf)); 
               if (events[i].data.fd == sock) {
                    // 接受服务器广告消息    
                    int ret = recv(sock, recv_buf, BUF_SIZE, 0);
                    // 清空结构体
                    memset(&msg, 0, sizeof(msg));
                    // 将消息转化为结构体
                    memcpy(&msg, recv_buf, sizeof(msg));
                    // ret = 0 服务端关闭
                    if (ret == 0) {
                        std::cout << "Server closed connection: " << sock << std::endl; 
                        close(sock);
                        isClinetwork = 0;
                    } else {
                        std::cout << msg.content << std::endl;
                    }
               } else {
                    // 父进程从管道中中读取数据
                    int ret = read(events[i].data.fd, recv_buf, BUF_SIZE);
                    if (ret == 0) {
                        isClinetwork = 0;         
                    } else {
                        // 将从管道中读取的字符串信息发送给服务端
                        send(sock, recv_buf, sizeof(recv_buf), 0);
                    }
               }
            } // for
        } // while
    }

    Close();
}
