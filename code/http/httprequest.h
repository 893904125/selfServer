//
// Created by szy on 2025/11/27.
//

#ifndef SELFSERVER_HTTPREQUEST_H
#define SELFSERVER_HTTPREQUEST_H
#include <unordered_map>
#include <unordered_set>
#include <algorithm>

#include "buffer.h"


class HttpRequest {
public:
    enum PARSE_STATE {
        REQUEST_LINE,   //请求行
        HEADERS,        //请求头
        BODY,           //数据
        FINISH          //完成
    };

    enum HTTP_CODE {

    };
    HttpRequest(){Init();};
    ~HttpRequest() = default;
    void Init();
    bool parse(Buffer &readBuffer);

    std::string& path();
    std::string path()const ;
    bool IsKeepAlive() const;
private:

    bool ParseRequestLine_(std::string &line);
    void ParsePath_();
    void ParseHeader_(std::string &line);
    void ParseBody_(std::string &line);

    void ParsePost_();
    void ParseFromUrlencoded_();


    bool UserVerify(const std::string &username, const std::string &password, bool isLog);

    PARSE_STATE state_;
    std::string method_, path_, version_, body_;
    std::unordered_map<std::string, std::string> header_;
    std::unordered_map<std::string, std::string> post_;
    static const std::unordered_set<std::string> DEFAULT_HTML;
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;
    static int ConverHex(char ch);
};


#endif //SELFSERVER_HTTPREQUEST_H