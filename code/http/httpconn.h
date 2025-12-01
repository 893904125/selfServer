//
// Created by szy on 2025/11/24.
//

#ifndef SELFSERVER_HTTPCONN_H
#define SELFSERVER_HTTPCONN_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <atomic>
#include "buffer/buffer.h"
#include "log/log.h"
#include "httprequest.h"
#include "httpresponse.h"

class HttpConn {
public:

    HttpConn();
    ~HttpConn();

    void init(int fd, struct sockaddr_in &addr);

    void Close();

    const char* GetIP()const;
    int GetPort()const;
    int GetFd()const;
    struct sockaddr_in GetAddr()const;

    ssize_t read(int *Errno);
    ssize_t write(int *Errno);

    bool process();

    int ToWriteBytes();

    bool IsKeepAlive();


    static bool isET;
    static const char* srcDir;
    static std::atomic<int> userCount;

private:
    int fd_;
    struct sockaddr_in addr_;
    bool isClose_;

    int iovCnt_;

    struct iovec iov_[2];

    Buffer readBuff_;
    Buffer writeBuff_;

    HttpRequest request_;
    HttpResponse response_;

};


#endif //SELFSERVER_HTTPCONN_H