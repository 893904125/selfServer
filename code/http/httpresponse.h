//
// Created by szy on 2025/11/27.
//

#ifndef SELFSERVER_HTTPRESPONSE_H
#define SELFSERVER_HTTPRESPONSE_H

#include <string>
#include <sys/stat.h>
#include <unordered_map>
#include "buffer.h"
#include "log.h"
#include <sys/mman.h>


class HttpResponse {
public:
    HttpResponse();
    ~HttpResponse();

    void Init(const std::string & srcDir, std::string&path, bool isKeepAlive, int code);

    void MakeResponse(Buffer& buff);
    size_t FileLen() const;
    char* File();
    void UnmapFile();

    void ErrorContent(Buffer& buff, std::string message);

    int Code() const{return code_;}

private:
    void AddStateLine_(Buffer& buff);
    void AddHeader_(Buffer& buff);
    void AddContent_(Buffer& buff);

    void ErrorHtml_();
    std::string GetFileType_();

    int code_;
    bool isKeepAlive_;
    std::string path_;
    std::string srcDir_;

    /*
     *负责拿文件内容，指向内存地址的指针，通过mmap，
     *把磁盘上的文件映射到进程的虚拟内存空间
     */
    char* mmFile_;
    //负责拿文件信息，用来存放文件的元数据（Metadata）。
    struct stat mmFileStat_;

    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    static const std::unordered_map<int, std::string> CODE_STATUS;
    static const std::unordered_map<int, std::string> CODE_PATH;
};


#endif //SELFSERVER_HTTPRESPONSE_H