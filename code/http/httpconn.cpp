//
// Created by szy on 2025/11/24.
//

#include "httpconn.h"

#include <arpa/inet.h>

const char* HttpConn::srcDir;
std::atomic<int> HttpConn::userCount;
bool HttpConn::isET;

HttpConn::HttpConn() {
    fd_ = 0;
    addr_ = {0};
    isClose_ = false;
}

HttpConn::~HttpConn() {
    Close();
}

void HttpConn::init(int fd, struct sockaddr_in &addr) {
    assert(fd > 0);
    //绑定连接的fd，addr
    userCount++;
    fd_ = fd;
    addr_ = addr;
    //清空缓冲区
    readBuff_.RetrieveAll();
    writeBuff_.RetrieveAll();
    isClose_ = false;
    LOG_INFO("Client[%d](%s:%d) in, userCount:%d", fd_, GetIP(), GetPort(), (int)userCount);
}

void HttpConn::Close() {
    // todo 其他资源释放
    if (isClose_ == false) {
        isClose_ = true;
        userCount--;
        close(fd_);
        LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d", fd_, GetIP(), GetPort(), (int)userCount);
    }
}

const char* HttpConn::GetIP() const {
    return inet_ntoa(addr_.sin_addr);
}

int HttpConn::GetPort() const {
    return addr_.sin_port;
}

int HttpConn::GetFd() const {
    return fd_;
}

ssize_t HttpConn::read(int *Errno) {
    /*使用buffer循环读取socket中的数据*/
    ssize_t len = -1;
    do {
        len = readBuff_.ReadFd(fd_, Errno);
        if (len <= 0) {
            break;
        }
    }while (isET);
    return len;
}

ssize_t HttpConn::write(int *writeErrno) {
    ssize_t len = -1;
    do {
        len = writev(fd_, iov_, iovCnt_);
        if (len <= 0) {
            *writeErrno = errno;
            break;
        }
        //传输完成
        if (iov_[0].iov_len + iov_[1].iov_len == 0) {break;}
        //传输长度大于第一个块，第一个块传输完成
        else if (static_cast<size_t>(len) > iov_[0].iov_len) {
            //计算第二个块发送了多少数据，并进行地址偏移
            iov_[1].iov_base = (uint8_t*)iov_[1].iov_base + (len - iov_[0].iov_len);
            iov_[1].iov_len = len - iov_[0].iov_len;
            // 如果 iov_[0] 还没归零（说明这是刚刚跨越边界的一刻）
            if (iov_[0].iov_len) {
                writeBuff_.RetrieveAll();
                iov_[0].iov_len = 0;
            }
        }else {
            //这次发送的数据量 <= 第一个缓冲区的长度
            //更新起始地址
            iov_[0].iov_base = (uint8_t*)iov_[0].iov_base + len;
            //更新剩余长度
            iov_[0].iov_len -= len;
            writeBuff_.Retrieve(len);
        }
        // 如果是 ET 模式（Edge Triggered），必须循环写直到写不出数据 (EAGAIN)
        // 或者如果还有很多数据没写完 (> 10240 字节)，也继续写，减少 epoll 触发次数
    }while(isET || ToWriteBytes() > 10240);
    return len;
}

bool HttpConn::process() {
    //1.处理请求
    request_.Init();
    if (readBuff_.ReadableBytes() <= 0) {
        return false;
    }else if (request_.parse(readBuff_)) {
        LOG_DEBUG("%s", request_.path().c_str());
        response_.Init(srcDir, request_.path(), request_.IsKeepAlive(), 200);
    }else {
        response_.Init(srcDir, request_.path(), false, 400);
    }

    //2.构建回复
    // 2.1 把回复写在buffer中
    response_.MakeResponse(writeBuff_);
    /*响应头*/
    iov_[0].iov_base = const_cast<char*>(writeBuff_.Peek());
    iov_[0].iov_len = writeBuff_.ReadableBytes();
    iovCnt_ = 1;

    /*body，返回文件*/
    if (response_.FileLen() > 0 && response_.File()) {
        iov_[1].iov_base = response_.File();
        iov_[1].iov_len = response_.FileLen();
        iovCnt_ = 2;
    }
    LOG_DEBUG("filesize:%d, %d  to %d", response_.FileLen() , iovCnt_, ToWriteBytes());
    return true;
}

int HttpConn::ToWriteBytes() {
    return iov_[0].iov_len + iov_[1].iov_len;
}

bool HttpConn::IsKeepAlive() {
    return request_.IsKeepAlive();
}

struct sockaddr_in HttpConn::GetAddr() const {
    return addr_;
}




