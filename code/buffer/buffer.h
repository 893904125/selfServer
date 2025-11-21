//
// Created by szy on 2025/11/20.
//

#ifndef SELFSERVER_BUFFER_H
#define SELFSERVER_BUFFER_H
#include <string>
#include <atomic>
#include <vector>
#include <assert.h>
#include <cstring>
#include <sys/uio.h>
#include <unistd.h>
class Buffer {
public:
    Buffer(int initBufferSize = 1024);
    ~Buffer() = default;

    size_t WritableBytes() const;
    size_t PrepableBytes() const;
    size_t ReadableBytes() const;

    const char* Peek() const;
    void EnsureWritable(size_t len);
    void HasWritten(size_t len);

    char* BeginWrite();
    const char* BeginWriteConst() const;

    void Retrieve(size_t len);
    void RetrieveAll();
    std::string RetrieveAllToStr();

    void Append(const std::string &str);
    void Append(const char* str, size_t len);
    void Append(const void* data, size_t len);
    void Append(const Buffer& buffer);

    ssize_t ReadFd(int fd, int* Errno);
    ssize_t WriteFd(int fd, int* Errno);


private:
    char* BeginPtr_();
    const char* BeginPtr_() const;
    void makeSpace_(size_t len);

    std::vector<char> buffer_;
    std::atomic<int> readPos_;
    std::atomic<int> writePos_;
};


#endif //SELFSERVER_BUFFER_H