#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <list>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Server ip
#define SERVER_IP "127.0.0.1"

// Server port
#define SERVER_PORT 8800

// int epoll_create(int size)
// maxiumn epoll handlers 
#define EPOLL_SIZE 5000

//buffer size
#define BUF_SIZE 0xFFFF

// Welcome meassage
#define SERVER_WELCOME "Welcome you join to the chat room! Your chat ID is: Client #%d"

// other meassage
#define SERVER_MESSAGE "ClientID %d say >> %s"
#define SERVER_PRIVATE_MESSAGE "Client %d say to you privately >> %s"
#define SERVER_PRIVATE_ERROR_MESSAGE "Client %d is not in the chat room yet~"

//Exit
#define EXIT "EXIT"

//Caution
#define CAUTION "There is only one int the char room!"

// 注册新的fd到epollfd中
// 参数enable_et表示是否启用ET模式，如果为True则启用，否则使用LT模式
static void addfd(int epollfd, int fd, bool enable_et) {
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN;
    if (enable_et) {
        ev.events =  EPOLLIN | EPOLLET    ;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
    // 设置socket为非阻塞模式
    // 套接字立刻返回，不管I/O是否完成，该函数所在的线程会继续运行
    //eg. 在recv(fd...)时，该函数立刻返回，在返回时，内核数据还没准备好会返回WSAEWOULDBLOCK错误代码
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
    printf("fd added to epoll!\n\n");
}

//定义传播信息的结构体
struct Msg {
    int type;
    int fromID;
    int toID;
    char content[BUF_SIZE];
};

#endif
