//
// Created by szy on 2025/11/24.
//

#ifndef SELFSERVER_SQLCONNPOOL_H
#define SELFSERVER_SQLCONNPOOL_H
#include <mysql/mysql.h>
#include <queue>
#include <mutex>
#include <semaphore.h>
#include <config/ConfigMgr.h>
#include <log/log.h>

/*
 * 设计为单例模式
 *  - MAX_CONN_ 记录最大的连接数
 *  - useCount_ 记录正在使用的连接
 *  - freeCount_ 记录空闲的连接
 *  - 用一个queue 保存连接
 *  - semId_ 用来控制连接池
 */
class SqlConnPool {
public:
      static SqlConnPool* Instance();

      MYSQL* GetConn();

      void FreeConn(MYSQL* sql);

      int GetFreeConnCount();

      void ClosePool();
private:
      SqlConnPool();
      ~SqlConnPool();

      int MAX_CONN_;
      int useCount_;
      int freeCount_;

      std::queue<MYSQL*> connQue_;
      std::mutex mtx_;
      sem_t semId_;
};

class SqlConnRAII {
public:
      SqlConnRAII(MYSQL** sql, SqlConnPool* connpool) {
            assert(connpool);
            *sql = connpool->GetConn();
            sql_ = *sql;
            connpool_ = connpool;
      }

      ~SqlConnRAII() {
            if (sql_) {connpool_->FreeConn(sql_);}
      }

private:
      MYSQL* sql_;
      SqlConnPool* connpool_;
};

#endif //SELFSERVER_SQLCONNPOOL_H