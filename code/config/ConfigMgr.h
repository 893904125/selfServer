//
// Created by szy on 2025/11/21.
//

#ifndef SELFSERVER_CONFIGMGR_H
#define SELFSERVER_CONFIGMGR_H
#include <map>
#include <string>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include "const.h"

struct SectionInfo {
    SectionInfo(){}
    SectionInfo(const SectionInfo& src) {
        section_data_ = src.section_data_;
    }
    SectionInfo(std::map<std::string, std::string>& src) {
        section_data_ = src;
    }

    SectionInfo& operator=(const SectionInfo& src) {
        if (&src == this) return *this;
        section_data_ = src.section_data_;
        return *this;
    }

    std::string operator[](const std::string& key) {
        if (section_data_.find(key) == section_data_.end()) return "";
        return section_data_[key];
    }

    std::map<std::string, std::string> section_data_;
};


class ConfigMgr {
public:
    static ConfigMgr& Instance();
    ~ConfigMgr();

    ConfigMgr& operator=(const ConfigMgr& src);

    //const std::string& 常量引用，这样既能接收临时对象，又能避免拷贝
    SectionInfo operator[](const std::string& section);


private:
    ConfigMgr();

    std::map<std::string, SectionInfo> configs_data_;
};

#endif //SELFSERVER_CONFIGMGR_H