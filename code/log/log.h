//
// Created by szy on 2025/11/20.
//

#ifndef SELFSERVER_LOG_H
#define SELFSERVER_LOG_H
#include <string>
#include "../buffer/buffer.h"
#include "BlockDeque.h"
#include <memory>
#include <thread>
#include <time.h>
#include <sys/stat.h>
#include <stdarg.h>

class Log {
public:
    /*获取单例*/
    static Log *Instance();

    /*
     * level: 设置日志等级
     * path: 文件路径
     * suffix: 文件后缀
     * maxQueueCapacity: 队列的大小，为0 是关闭异步
     */
    void init(int level = 1, const char *path = "./log", const char *suffix = ".log", int maxQueueCapacity = 1024);

    /*写日志*/
    void write(int level, const char *format, ...);

    /*强制刷新，确保缓冲区数据真正写到磁盘里了*/
    void flush();

    //传递给thread的回调函数
    static void FlushLogThread();

    /*动态调整和查询日志等级*/
    int GetLevel();

    void SetLevel(int level);

    bool IsOpen();

private:
    Log();

    virtual ~Log();

    /*根据等级向buffer中追加字符串*/
    void AppendLogLevelTitle_(int level);

    /*消费者主线程*/
    void AsyncWrite_();

private:
    static const int LOG_PATH_LEN = 256;
    static const int LOG_NAME_LEN = 256;
    //用来标记一个文件写这么多行日志，防止单个日志文件过长
    static const int MAX_LINES = 50000;

    //基础配置与状态
    const char *path_;
    const char *suffix_;
    int level_;
    bool isOpen_;

    //日志轮转控制， 超过了MAX_LINES，或者新的一天，创建新的日志文件
    int lineCount_;
    int toDay_;

    //核心缓冲区，
    Buffer buffer_;

    //异步写入机制
    bool isAsync_;
    std::unique_ptr<BlockDeque<std::string> > deque_;
    std::unique_ptr<std::thread> writeThread_;

    //线程安全与文件资源
    std::mutex mutex_;
    FILE *fp_;
};

#define LOG_BASE(level, format, ...) \
    do {\
        Log* log = Log::Instance();\
        if (log->IsOpen() && log->GetLevel() <= level) {\
            log->write(level, format, ##__VA_ARGS__); \
            log->flush();\
        }\
    } while(0);

#define LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);
#define LOG_INFO(format, ...) do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0);
#define LOG_WARN(format, ...) do {LOG_BASE(2, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) do {LOG_BASE(3, format, ##__VA_ARGS__)} while(0);

#endif //SELFSERVER_LOG_H
