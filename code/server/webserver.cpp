//
// Created by szy on 2025/11/24.
//

#include "webserver.h"

// 服务器初始化
WebServer::WebServer(int port, int trigMode, int timeoutMs, bool optLinger, int threadNum, bool openLog, int logLevel,
    int logQueSize) :
    port_(port), optLinger_(optLinger), timeoutMs_(timeoutMs), isClose_(false),
    listenEvent_(0), connEvent_(0), timer_(new HeapTimer()),
    threadpool_(new ThreadPool(threadNum)), epoller_(new Epoller())
{
    // 1. 初始化资源路径
    srcDir_ = getcwd(nullptr, 256);
    assert(srcDir_);
    strncat(srcDir_, "/resources/", 16);

    //2. 初始化HttpConn的参数
    HttpConn::userCount = 0;
    HttpConn::srcDir = srcDir_;

    // 3. 初始化epoll的模式
    InitEpollEventMode_(trigMode);

    // 4. 初始化 socket,绑定监听端口
    if (!InitSocket_()){isClose_ = true;}

    // 5. 启动日志系统
    if (openLog) {
        Log::Instance()->init(logLevel, "./log", ".log", logQueSize);
        if (isClose_) {LOG_ERROR("============ Server init error! =============");}
        else {
            LOG_INFO("========== Server init ==========");
            LOG_INFO("Port:%d, OpenLinger: %s", port_, optLinger? "true":"false");
            LOG_INFO("Listen Mode: %s, OpenConn Mode: %s",
                            (listenEvent_ & EPOLLET ? "ET": "LT"),
                            (connEvent_ & EPOLLET ? "ET": "LT"));
            LOG_INFO("LogSys level: %d", logLevel);
            LOG_INFO("srcDir: %s", HttpConn::srcDir);
            LOG_INFO("ThreadPool num: %d", threadNum);
        }
    }

    // 6.初始化MysqlConnPool
    SqlConnPool::Instance();
}


WebServer::~WebServer() {
    close(listenFd_);
    isClose_ = true;
    free(srcDir_);
    SqlConnPool::Instance()->ClosePool();
}

void WebServer::Start() {
    int timeMs = -1;
    if(!isClose_) { LOG_INFO("========== Server start =========="); }
    while (!isClose_) {
        if (timeoutMs_ > 0) {
            timeMs = timer_->GetNextTick();
        }
        //获取epoll的等待队列
        int eventCount = epoller_->Wait(timeMs);

        for (int i =0; i < eventCount; ++i) {
            int fd = epoller_->GetEventFd(i);
            uint32_t event = epoller_->GetEvent(i);
            if (fd == listenFd_) {
                DealListen_();
            }
            else if (event & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                assert(users_.size() > 0);
                CloseConn_(&users_[fd]);
            }
            else if (event & EPOLLIN) {
                assert(users_.size() > 0);
                DealRead_(&users_[fd]);
            }
            else if (event & EPOLLOUT) {
                assert(users_.size() > 0);
                DealWrite_(&users_[fd]);
            }else {
                LOG_ERROR("Unexpected event");
            }
        }
    }
}

bool WebServer::InitSocket_() {
    int ret;
    // 1.设置监听的协议，地址，端口
    if (port_ > 65535 || port_ < 1024) {
        LOG_ERROR("INVALID PORT");
        return false;
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_);

    // 2.设置优雅关闭：直到所剩数据发送完毕或超时
    struct linger optLinger;
    if (optLinger_) {
        optLinger.l_linger = 1;
        optLinger.l_onoff = 1;
    }

    // 3.创建listenFd
    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0) {
        LOG_ERROR("Create socket error!", port_);
        return false;
    }

    // 4.设置socket选项
    //  - SO_LINGER 控制close行为,当你调用 close 时，如果发送缓冲区里还有数据，进程会阻塞（等一下）。
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
    if (ret < 0) {
        LOG_ERROR("Init linger error!", port_);
        return false;
    }

    /* 端口复用 */
    /* 只有最后一个套接字会正常接收数据。 */
    //  - SO_REUSEADDR 允许端口快速复用 服务器重启可以立即重新绑定端口
    int optval = 1;
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));
    if (ret == -1) {
        LOG_ERROR("set socket setsocketopt error !");
        close(listenFd_);
        return false;
    }

    // 5.绑定套接字端口和ip
    ret = bind(listenFd_, (struct sockaddr*)&addr, sizeof(addr));
    if (ret < 0) {
        LOG_ERROR("Bind Port:%d error!", port_);
        close(listenFd_);
        return false;
    }

    // 6.开启监听，监听队列1024
    ret = listen(listenFd_, 1024);
    if(ret < 0) {
        LOG_ERROR("Listen port:%d error!", port_);
        close(listenFd_);
        return false;
    }

    // 7.加入epoll事件
    ret = epoller_->AddFd(listenFd_, listenEvent_ | EPOLLIN);
    if (ret == 0) {
        LOG_ERROR("Add listen error!");
        close(listenFd_);
        return false;
    }

    // 8.把监听的文件描述符设置成非阻塞，实现高并发
    SetFdNonBlock_(listenFd_);
    LOG_INFO("Server port:%d", port_);
    return true;
}

void WebServer::InitEpollEventMode_(int trigMode) {
    listenEvent_ |= EPOLLRDHUP;
    /* EPOLLONESHOT 保证任何时刻，一个socket只能被一个线程拥有，一定要记得重置不然这个soket就只能等死了*/
    connEvent_ |= EPOLLONESHOT | EPOLLRDHUP;
    switch (trigMode) {
        case 0:
            break;
        case 1:
            listenEvent_ |= EPOLLET;
            break;
        case 2:
            connEvent_ |= EPOLLET;
            break;
        case 3:
            listenEvent_ |= EPOLLET;
            connEvent_ |= EPOLLET;
            break;
        default:
            listenEvent_ |= EPOLLET;
            connEvent_ |= EPOLLET;
            break;
    }
    HttpConn::isET = (connEvent_ & EPOLLET);
}

int WebServer::SetFdNonBlock_(int fd) {
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}


void WebServer::DealListen_() {
    //用 do-while，一直循环的读取请求队列的数据
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    /* ET模式，内核通知读取数据，需要循环读取，直到把数据读取完毕 */
    do {
        int fd = accept(listenFd_, (struct sockaddr*)&addr, &len);
        //但取到的fd < 0 表示没有连接了，return掉
        if (fd < 0){return;}
        else if (HttpConn::userCount >= MAX_FD){
            SendError_(fd, "Server busy!");
            LOG_WARN("Client is full!");
            return;
        }
        AddClient_(fd, addr);
    }while (listenEvent_ & EPOLLET);
}

void WebServer::DealRead_(HttpConn *client) {
    /* 调用工作线程处理读事件，同时更新定时器时间 */
    assert(client);
    ExtentTime_(client);
    threadpool_->AddTask(std::bind(&WebServer::OnRead_, this,client));
}

void WebServer::DealWrite_(HttpConn *client) {
    /*调用工作线程处理写事件，更新定时器*/
    assert(client);
    ExtentTime_(client);
    threadpool_->AddTask(std::bind(&WebServer::OnWrite_, this, client));
}

void WebServer::OnRead_(HttpConn *client) {
    /*处理读事件的回调函数*/
    //调用连接读取数据
    assert(client);
    int ret = -1;
    int readErrno = 0;
    ret = client->read(&readErrno);
    if (ret < 0 && readErrno != EAGAIN) {
        CloseConn_(client);
        return;
    }
    OnProcess(client);
}

void WebServer::OnWrite_(HttpConn *client) {
    assert(client);
    int ret = -1;
    int writeErrno = 0;
    ret = client->write(&writeErrno);
    if (client->ToWriteBytes() == 0) {
        //传输完成
        //如果用户请求了keep-Alive，不能关闭连接
        if (client->IsKeepAlive()) {
            OnProcess(client);
            return;
        }
    }else if (ret < 0) {
        //如果是内存缓存区满了造成的错误
        if (writeErrno == EAGAIN) {
            /* 继续传输 */
            // 重新注册 EPOLLOUT 事件，并且带上 EPOLLONESHOT (在 connEvent_ 里)
            epoller_->ModFd(client->GetFd(), EPOLLOUT | connEvent_);
            return;
        }
    }
    CloseConn_(client);
}

void WebServer::OnProcess(HttpConn *client) {
    //如果process返回true， 代表HTTP请求解析成功，且相应报文准备好了
    if (client->process()) {
        //注册EPOLLOUT事件，让主线程监听“可写事件”，把相应返回给客服端
        //！！！这里还包含了EPOLLIN的重置
        epoller_->ModFd(client->GetFd(), EPOLLOUT | connEvent_);
    }else {
        //返回false; 接收到的请求数据不完整，注册EPOLLIN继续接收
        epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLIN);
    }
}

void WebServer::SendError_(int fd, const char *info) {
    assert(fd > 0);
    int ret = send(fd, info, strlen(info), 0);
    if (ret < 0) {
        LOG_WARN("send error to client[%d] error!", fd);
    }
    close(fd);
}

void WebServer::ExtentTime_(HttpConn *client) const {
    assert(client);
    if (timeoutMs_ > 0) {timer_->adjust(client->GetFd(), timeoutMs_);}
}

/* 添加连接，把连接放到epoll中 */
void WebServer::AddClient_(int fd, sockaddr_in addr) {
    assert(fd > 0);
    // 1. 把连接插入map中,管理连接者的文件描述符和ip
    users_[fd].init(fd, addr);
    // 2. 添加定时器
    if (timeoutMs_ > 0) {
        timer_->add(fd, timeoutMs_, std::bind(&WebServer::CloseConn_, this, &users_[fd]));
    }
    // 3. 添加epoll监听，监听客服端请求
    epoller_->AddFd(fd, EPOLLIN | connEvent_);
    // 4. 设置fd为非阻塞模式
    SetFdNonBlock_(fd);

    LOG_INFO("Client[%d] in!", users_[fd].GetFd());

}

void WebServer::CloseConn_(HttpConn *client) {
    assert(client);
    LOG_INFO("Client[%d] quit!", client->GetFd());
    epoller_->DelFd(client->GetFd());
    client->Close();
}
