//
// Created by szy on 2025/11/21.
//

#include "ConfigMgr.h"

ConfigMgr &ConfigMgr::Instance() {
    static ConfigMgr inst;
    return inst;
}

ConfigMgr::~ConfigMgr() {
    configs_data_.clear();
}

ConfigMgr &ConfigMgr::operator=(const ConfigMgr &src) {
    if (&src == this) return *this;
    configs_data_ = src.configs_data_;
    return *this;
}

SectionInfo ConfigMgr::operator[](const std::string &section) {
    if (configs_data_.find(section) == configs_data_.end()) return SectionInfo();
    return configs_data_[section];
}

ConfigMgr::ConfigMgr() {
    boost::filesystem::path current_dir = boost::filesystem::current_path();
    boost::filesystem::path config_path = current_dir / "config.ini";
    std::cout << config_path.string() << std::endl;

    boost::property_tree::ptree ptree;
    boost::property_tree::read_ini(config_path.string(), ptree);

    for (const auto& section_part: ptree) {
        const std::string section_name = section_part.first;
        auto section_params = section_part.second;
        std::map<std::string, std::string> section_data;
        for (const auto& param: section_params) {
            const std::string key = param.first;
            const std::string value = param.second.get_value<std::string>();
            section_data[key] = value;
        }
        configs_data_[section_name] = SectionInfo (section_data);
    }
}
