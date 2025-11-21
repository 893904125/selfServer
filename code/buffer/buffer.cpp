//
// Created by szy on 2025/11/20.
//

#include "buffer.h"


Buffer::Buffer(int initBufferSize) : buffer_(initBufferSize) {
    readPos_ = 0;
    writePos_ = 0;
}

size_t Buffer::ReadableBytes() const {
    return writePos_ - readPos_;
}

size_t Buffer::WritableBytes() const {
    return buffer_.size() - writePos_;
}

size_t Buffer::PrepableBytes() const {
    return readPos_;
}

const char *Buffer::Peek() const {
    return BeginPtr_() + readPos_;
}

void Buffer::EnsureWritable(size_t len) {
    if (len > WritableBytes()) {
        makeSpace_(len);
    }
    assert(len <= WritableBytes());
}

void Buffer::HasWritten(size_t len) {
    writePos_ += len;
}

char *Buffer::BeginWrite() {
    return BeginPtr_() + writePos_;
}
const char *Buffer::BeginWriteConst() const {
    return BeginPtr_() + writePos_;
}

void Buffer::Retrieve(size_t len) {
    assert(len <= ReadableBytes());
    readPos_ += len;
}

void Buffer::RetrieveAll() {
    memset(BeginPtr_(), 0, buffer_.size());
    readPos_ = 0;
    writePos_ = 0;
}

std::string Buffer::RetrieveAllToStr() {
    std::string str(Peek(), ReadableBytes());
    RetrieveAll();
    return str;
}

void Buffer::Append(const char* str, size_t len) {
    assert(str);
    EnsureWritable(len);
    std::copy(str, str + len, BeginWrite());
    HasWritten(len);
}

void Buffer::Append(const std::string &str) {
    Append(str.data(), str.length());
}

void Buffer::Append(const Buffer &buffer) {
    Append(buffer.Peek(), buffer.ReadableBytes());
}

void Buffer::Append(const void* data, size_t len) {
    assert(data);
    Append(static_cast<const char*>(data), len);
}

ssize_t Buffer::ReadFd(int fd, int* Errno) {
    char buff[65535];
    struct iovec iov[2];
    const size_t writable = WritableBytes();
    iov[0].iov_base = BeginWrite();
    iov[0].iov_len = writable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    const ssize_t len = readv(fd, iov, 2);
    if (len < 0) {
        *Errno = errno;
    }else if (static_cast<size_t>(len) <= writable) {
        HasWritten(len);
    }else {
        writePos_ = buffer_.size();
        Append(buff, len - writable);
    }
    return len;
}

ssize_t Buffer::WriteFd(int fd, int* Errno) {
    size_t readSize = ReadableBytes();
    ssize_t len = write(fd, Peek(), readSize);
    if (len < 0) {
        *Errno = errno;
    }
    readPos_ += len;
    return len;
}

char *Buffer::BeginPtr_() {
    return &*buffer_.begin();
}

const char *Buffer::BeginPtr_() const {
    return &*buffer_.begin();
}

void Buffer::makeSpace_(size_t len) {
    if (PrepableBytes() + WritableBytes() < len) {
        //开辟新空间
        buffer_.resize(writePos_ + len + 1);
    }else {
        size_t readable = ReadableBytes();
        std::copy(Peek(), Peek() + readable, BeginPtr_());
        readPos_ = 0;
        writePos_ = readPos_ + readable;
        assert(readable == ReadableBytes());
    }
}


