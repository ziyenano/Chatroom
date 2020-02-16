#ifndef SERVER_H
#define SERVER_H

#include <string>
#include "common.h"

class Server {
public:
    Server();
   
    // 初始化服务端设置
    void Init();

    // 关闭服务
    void Close();

    // 启动服务端
    void Start();

private:
    // 广告消息给所有客户端
    int SendBroadcastMessage(int clientfd);
    // 服务端serverAddr信息
    struct sockaddr_in serverAddr;
    // 监听socket
    int listener;
    // epoll_create 创建后返回值
    int epfd;
    //客户端列表
    std::list<int> clients_list;
};

#endif
