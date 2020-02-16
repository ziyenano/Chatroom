#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include "common.h"

class Client {
public:
    Client();
    
    // 连接服务器
    void Connect();

    // 断开连接
    void Close();

    // 启动客户端
    void Start();

private:
    // 当前链接服务器创建的socket
    int sock;
    // 当前进程
    int pid;
    // epoll_create创建后返回的值
    int epfd;
    // 创建管道, 其中fd[0]用于父进程读, fd[1]用于子进程写
    int pipe_fd[2];
    // 客户端是否正常工作
    bool isClinetwork;
    // 聊天信息
    Msg msg;
    // 结果提要转化字符串
    char send_buf[BUF_SIZE];    
    char recv_buf[BUF_SIZE];

    // 用于连接服务器的ip+port
    struct sockaddr_in serverAddr;
};

#endif
