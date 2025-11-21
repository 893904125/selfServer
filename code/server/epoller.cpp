//
// Created by szy on 2025/11/21.
//

#include "epoller.h"

#include <unistd.h>

Epoller::Epoller(int maxSize):epollFd_(epoll_create1(0)), events_(maxSize) {
    assert(epollFd_ > 0 && events_.size() > 0);
}

Epoller::~Epoller() {
    close(epollFd_);
}

bool Epoller::AddFd(int fd, uint32_t events) {
    if (fd < 0) return false;
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev);
}

bool Epoller::ModFd(int fd, uint32_t events) {
    if (fd < 0) return false;
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &ev);
}

bool Epoller::DelFd(int fd) {
    if (fd < 0) return false;
    epoll_event ev = {0};
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &ev);
}

int Epoller::Wait(int timeoutMs) {
    return epoll_wait(epollFd_, events_.data(), static_cast<int>(events_.size()), timeoutMs);
}

uint32_t Epoller::GetEvent(int i) {
    assert(0 < i && i < events_.size());
    return events_[i].events;
}

int Epoller::GetEventFd(int i) {
    assert(0 < i && i < events_.size());
    return events_[i].data.fd;
}

