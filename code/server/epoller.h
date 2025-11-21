//
// Created by szy on 2025/11/21.
//

#ifndef SELFSERVER_EPOLLER_H
#define SELFSERVER_EPOLLER_H

#include <sys/epoll.h>
#include <vector>
#include <assert.h>
class Epoller{
public:
    explicit Epoller(int maxSize = 1024);
    ~Epoller();


    bool AddFd(int fd, uint32_t events);
    bool ModFd(int fd, uint32_t events);
    bool DelFd(int fd);

    int Wait(int timeoutMs);

    int GetEventFd(int i);

    uint32_t GetEvent(int i);


private:
    //一个文件描述符保存epoll地址
    int epollFd_;

    //一个epoll事件vector 保存wait的epoll事件
    std::vector<epoll_event> events_;
};


#endif //SELFSERVER_EPOLLER_H