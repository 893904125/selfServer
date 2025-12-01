//
// Created by szy on 2025/11/24.
//

#include "SqlConnPool.h"

SqlConnPool* SqlConnPool::Instance(){
    static SqlConnPool instance;
    return &instance;
}

SqlConnPool::SqlConnPool() {
    auto cfg = ConfigMgr::Instance();
    std::string host = cfg["Mysql"]["Host"].c_str();
    std::string user = cfg["Mysql"]["User"].c_str();
    std::string passwd = cfg["Mysql"]["Passwd"].c_str();
    std::string database = cfg["Mysql"]["Schema"].c_str();
    int port = stoi(cfg["Mysql"]["Port"]);
    int connSize = stoi(cfg["Mysql"]["PoolSize"]);
    assert(connSize > 0);
    //创建连接放入queue
    for (int i = 0; i < connSize; i++) {
        MYSQL* sql = nullptr;
        sql = mysql_init(nullptr);
        if (!sql) {
            LOG_ERROR("mysql_init() failed");
            assert(sql);
        }
        sql = mysql_real_connect(sql, host.c_str(), user.c_str(),
                                passwd.c_str(), database.c_str(),
                                    port, nullptr, 0);
        if (!sql) {
            LOG_ERROR("mysql_real_connect() failed");
        }
        connQue_.push(sql);
    }
    LOG_INFO("mysql connect success, host: %s, user: %s, passwd: %s, database: %s", host.c_str(), user.c_str(), passwd.c_str(), database.c_str());
    MAX_CONN_ = connSize;
    useCount_ = 0;
    freeCount_ = MAX_CONN_;

    sem_init(&semId_, 0, MAX_CONN_);
}


MYSQL *SqlConnPool::GetConn() {
    MYSQL* sql =nullptr;
    if (connQue_.empty()) {
        LOG_WARN("connQue_ Busy");
    }
    sem_wait(&semId_);
    {
        std::lock_guard<std::mutex> locker(mtx_);
        sql = connQue_.front();
        connQue_.pop();
    }
    return sql;
}

void SqlConnPool::FreeConn(MYSQL* sql) {
    assert(sql);
    std::lock_guard<std::mutex>locker(mtx_);
    connQue_.push(sql);
    sem_post(&semId_);
}

int SqlConnPool::GetFreeConnCount() {
    std::lock_guard<std::mutex> locker(mtx_);
    return connQue_.size();
}

void SqlConnPool::ClosePool() {
    std::lock_guard<std::mutex> locker(mtx_);
    while (!connQue_.empty()) {
        auto item = connQue_.front();
        connQue_.pop();
        mysql_close(item);
    }
    //MySQL的总开关，释放资源
    mysql_library_end();
}

SqlConnPool::~SqlConnPool() {
    ClosePool();
}
