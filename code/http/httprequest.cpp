//
// Created by szy on 2025/11/27.
//

#include "httprequest.h"

#include <regex>

#include "log.h"
const std::unordered_set<std::string> HttpRequest::DEFAULT_HTML{
    "/index", "/register", "/login",
    "/welcome", "/video", "/picture",
};

const std::unordered_map<std::string, int > HttpRequest::DEFAULT_HTML_TAG{
    {"register.html", 0}, {"login.html", 1},
};

void HttpRequest::Init() {
    method_ = path_ = version_ = version_ = "";
    state_ = REQUEST_LINE;
    header_.clear();
    post_.clear();
}


bool HttpRequest::parse(Buffer &readBuffer) {
    const char CRLF[] = "\r\n";
    if (readBuffer.ReadableBytes() <= 0) {
        return false;
    }
    while (readBuffer.ReadableBytes() && state_ != FINISH) {
        const char* lineEnd = std::search(readBuffer.Peek(), readBuffer.BeginWriteConst(), CRLF, CRLF + 2);
        std::string line(readBuffer.Peek(), lineEnd);
        switch (state_) {
            case REQUEST_LINE:
                /*解析请求行*/
                // 比如：GET /index.html HTTP/1.1
                if (!ParseRequestLine_(line)) {
                    return false;
                }
                /*解析请求路径*/
                ParsePath_();
                break;
            case HEADERS:
                /*解析请求头*/
                ParseHeader_(line);
                if (readBuffer.ReadableBytes() <= 2) {
                    state_ = FINISH;
                }
                break;
            case BODY:
                /*解析消息体*/
                ParseBody_(line);
                break;
            default:
                break;
        }
        /*消息读完了*/
        if (lineEnd == readBuffer.BeginWrite()) {break;}
        //未读完，跳过/r/n继续读
        readBuffer.RetrieveUntil(lineEnd + 2);
    }
    LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(), version_.c_str());
    return true;
}


std::string & HttpRequest::path() {
    return path_;
}


std::string HttpRequest::path() const {
    return path_;
}


bool HttpRequest::IsKeepAlive() const {
    if (header_.count("Connection") == 1) {
        return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
    }
    return false;
}


bool HttpRequest::ParseRequestLine_(std::string &line) {
    /*GET /index.html HTTP/1.1*/
    std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch subMatch;
    if (std::regex_match(line, subMatch ,patten)) {
        method_ = subMatch[1];
        path_ = subMatch[2];
        version_ = subMatch[3];
        state_ = HEADERS;
        return true;
    }
    LOG_ERROR("RequestLine Error");
    return false;
}


void HttpRequest::ParsePath_() {
    if (path_ == "/") {
        path_ = "/index.html";
    }
    else {
        for (auto& item : HttpRequest::DEFAULT_HTML) {
            if (item == path_) {
                path_ += ".html";
                break;
            }
        }
    }
}


void HttpRequest::ParseHeader_(std::string &line) {
    std::regex patten("^([^:]*): ?(.*)$");
    std::smatch subMatch;
    if (std::regex_match(line, subMatch, patten)) {
        header_[subMatch[1]] = subMatch[2];
    }else {
        state_ = BODY;
    }
}


void HttpRequest::ParseBody_(std::string &line) {
    body_ = line;
    ParsePost_();
    state_ = FINISH;
    LOG_DEBUG("Body:%s, len:%d", line.c_str(), line.size());
}

void HttpRequest::ParsePost_() {
    if (method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded") {
        ParseFromUrlencoded_();
        if (DEFAULT_HTML_TAG.count(path_)) {
            int tag = DEFAULT_HTML_TAG.find(path_)->second;
            LOG_DEBUG("tag:%d",tag);
            if (tag == 0 || tag == 1) {
                bool isLogin = (tag == 1);
                if (UserVerify(post_["username"], post_["password"], isLogin)) {
                    path_= "/welcome.html";
                }else {
                    path_ = "/error.html";
                }
            }
        }
    }
}

//key1=value1&key2=value2
void HttpRequest::ParseFromUrlencoded_() {
    if (body_.size() == 0){return;}
    std::string key, value;
    int num = 0;
    int n = body_.size();
    int i = 0, j = 0;
    for (; i < n; i++) {
        char ch = body_[i];
        switch (ch) {
            case '=':
                key = body_.substr(j, i - j);
                j = i + 1;
                break;
            case '+':
                body_[i] = ' ';
                break;
            case '%':
                num = ConverHex(body_[i + 1]) * 16 + ConverHex(body_[i + 2]);
                body_[i + 2] = num % 10 + '0';
                body_[i + 1] = num / 10 + '0';
                i += 2;
                break;
            case '&':
                value = body_.substr(j, i - j);
                j = i + 1;
                post_[key] = value;
                LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
                break;
            default:
                break;
        }
        assert(j <= i);
        if (post_.count(key) == 0 && j < i) {
            value = body_.substr(j, i - j);
            post_[key] = value;
        }
    }
}

bool HttpRequest::UserVerify(const std::string &username, const std::string &password, bool isLog) {
    //todo 用户登录
    return true;
}

//十六进制 -> 十进制
int HttpRequest::ConverHex(char ch) {
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    return ch;
}

