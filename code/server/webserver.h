//
// Created by szy on 2025/11/24.
//

#ifndef SELFSERVER_WEBSERVER_H
#define SELFSERVER_WEBSERVER_H

#include <cstdint>
#include "pool/SqlConnPool.h"
#include "pool/ThreadPool.h"
#include <memory>
#include "timer/heaptimer.h"
#include "server/epoller.h"
#include "http/httpconn.h"
#include <unordered_map>
#include <log/log.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>

class WebServer {
public:
    /*
     * port 监听端口
     * trigMode epoll模式
     * timeoutMs 连接超时时间
     * OptLinger 是否优雅退出
     * threadNum 线程池数量
     * openLog logLevel logQueSize 日志系统参数
     */
    WebServer(int port, int trigMode, int timeoutMs, bool optLinger,
        int threadNum, bool openLog, int logLevel, int logQueSize);

    ~WebServer();

    void Start();
private:
    bool InitSocket_();
    void InitEpollEventMode_(int mode);
    int SetFdNonBlock_(int fd);

    void DealListen_();
    void DealRead_(HttpConn *client);
    void DealWrite_(HttpConn *client);

    void OnRead_(HttpConn *client);
    void OnWrite_(HttpConn *client);
    void OnProcess(HttpConn *client);


    void SendError_(int fd, const char* info);
    void ExtentTime_(HttpConn *client) const;

    void AddClient_(int fd, sockaddr_in addr);


    void CloseConn_(HttpConn *client);

    static const int MAX_FD = 65536;
    int port_;
    bool optLinger_;
    int timeoutMs_;
    bool isClose_;
    int listenFd_;
    char* srcDir_;

    uint32_t listenEvent_;
    uint32_t connEvent_;

    std::unique_ptr<HeapTimer> timer_;
    std::unique_ptr<ThreadPool> threadpool_;
    std::unique_ptr<Epoller> epoller_;
    std::unordered_map<int, HttpConn> users_;

};


#endif //SELFSERVER_WEBSERVER_H