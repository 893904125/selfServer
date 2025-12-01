//
// Created by szy on 2025/11/21.
//

#ifndef SELFSERVER_EPOLLER_H
#define SELFSERVER_EPOLLER_H

#include <sys/epoll.h>
#include <vector>
#include <assert.h>

/*
 *Epoll 有两种工作模式
 *LT(Level Triggered) 水平触发
 *  - 数据到内核缓存区时，只要数据可读，就会一直通知。不用担心漏掉数据，稳定。
 *  - 但并发量提高了，无效的通知会增多，性能会有所下降。
 *ET(Edge Triggered) 边缘触发
 *  - 内核只在状态变化的时候通知一次，减少系统通知的次数，比较高效
 *  - 但需要处理边界情况。
 *
 */
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