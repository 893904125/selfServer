//
// Created by szy on 2025/11/20.
//

#include "log.h"


Log::Log() {
    lineCount_ = 0;
    isAsync_ = false;
    toDay_ = 0;
    deque_ = nullptr;
    writeThread_ = nullptr;
    fp_ = nullptr;
}

Log* Log::Instance() {
    static Log inst;
    return &inst;
}

/*
 * 1 创建deque 和 thread
 * 2 根据时间创建文件名
 * 3 创建文件描述符
 */
void Log::init(int level,const char* path, const char* suffix, int maxQueueCapacity) {
    isOpen_ = true;
    level_ = level;
    if (maxQueueCapacity > 0) {
        isAsync_ = true;
        if (!deque_) {
            std::unique_ptr<BlockDeque<std::string>> deque(new BlockDeque<std::string>(maxQueueCapacity));
            deque_ = std::move(deque);

            std::unique_ptr<std::thread> wThread(new std::thread(FlushLogThread));
            writeThread_= std::move(wThread);
        }
    }else {
        isAsync_ = false;
    }

    time_t timer = time(nullptr);
    struct tm t;
    localtime_r(&timer, &t); // 线程安全

    path_ = path;
    suffix_ = suffix;
    char fileName[LOG_NAME_LEN] = {0};
    snprintf(fileName, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s",
        path_, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, suffix_);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        //清空缓冲区
        buffer_.RetrieveAll();
        //如果已经存在文件描述符，先让其输出完成在关闭
        if (fp_) {
            flush();
            fclose(fp_);
        }
        fp_ = fopen(fileName, "a");
        if (fp_ == nullptr) {
            mkdir(path_, 0777);
            fp_ = fopen(fileName, "a");
        }
        assert(fp_ != nullptr);
    }

}

/*
 * 写日志的核心函数
 * 1 准备和日志轮转
 * 2 格式化日志内容，拼接到buffer中
 * 3 根据是放入队列中异步写，还是直接写文件
 * 异步写就是把数据放在缓存区，同步写就是直接写
 */
void Log::write(int level, const char *format, ...) {
    //获得高精度时间
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);
    time_t tSec = now.tv_sec;
    struct tm *sysTem = localtime(&tSec);
    struct tm t = *sysTem;
    va_list valist;

    //根据日期和行数判断是否应该创建新的日志文件
    if (toDay_ != t.tm_mday || (lineCount_ && (lineCount_ % MAX_LINES == 0))) {
        std::unique_lock<std::mutex> locker(mutex_);
        locker.unlock();

        char newFile[LOG_NAME_LEN];
        char tail[36] = {0};
        snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
        if (toDay_ != t.tm_mday) { //如果日期变了，就创建新的文件
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s%s", path_, tail, suffix_);
            toDay_ = t.tm_mday;
            lineCount_ = 0;
        }else { //日期没变，创建新文件，并在后面添加后缀
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s-%d%s", path_, tail, (lineCount_ / MAX_LINES),suffix_);
        }

        locker.lock();
        flush();
        fclose(fp_);
        fp_ = fopen(newFile, "a");
        assert(fp_ != nullptr);
    }

    {
        std::unique_lock<std::mutex>locker(mutex_);
        lineCount_++;

        int n = snprintf(buffer_.BeginWrite(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                    t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                    t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
        buffer_.HasWritten(n);

        AppendLogLevelTitle_(level);

        //写入用户内容（处理可变参数）
        va_start(valist, format);
        int m = vsnprintf(buffer_.BeginWrite(), buffer_.WritableBytes(), format, valist);
        va_end(valist);

        buffer_.HasWritten(m);
        buffer_.Append("\n\0", 2);

        if (isAsync_ && deque_ && !deque_->full()) {
            deque_->push_back(buffer_.RetrieveAllToStr());
        }else {
            fputs(buffer_.Peek(), fp_);
        }

        //清空buffer，为下一条日志做准备
        buffer_.RetrieveAll();
    }
}


/*根据等级向buffer中追加字符串*/
void Log::AppendLogLevelTitle_(int level) {
    switch(level) {
        case 0:
            buffer_.Append("[debug]: ", 9);
            break;
        case 1:
            buffer_.Append("[info] : ", 9);
            break;
        case 2:
            buffer_.Append("[warn] : ", 9);
            break;
        case 3:
            buffer_.Append("[error]: ", 9);
            break;
        default:
            buffer_.Append("[info] : ", 9);
            break;
    }
}

int Log::GetLevel() {
    std::lock_guard<std::mutex> lock(mutex_);
    return level_;
}
void Log::SetLevel(int level) {
    std::lock_guard<std::mutex> lock(mutex_);
    level_ = level;
}

bool Log::IsOpen() {
    std::lock_guard<std::mutex> lock(mutex_);
    return isOpen_;
}

void Log::flush() {
    if (isAsync_) {
        deque_->flush();
    }
    fflush(fp_);
}

void Log::AsyncWrite_() {
    std::string str = "";
    while (deque_->pop(str)) {
        std::lock_guard<std::mutex> lock(mutex_);
        fputs(str.c_str(), fp_);
    }
}

void Log::FlushLogThread() {
    Log::Instance()->AsyncWrite_();
}

Log::~Log() {
    if (writeThread_ && writeThread_->joinable()) {
        while (!deque_->empty()) {
            deque_->flush();
        }
        deque_->Close();
        writeThread_->join();
    }
    if (fp_) {
        std::lock_guard<std::mutex> lock(mutex_);
        flush();
        fclose(fp_);
    }
}
