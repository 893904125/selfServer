#include <iostream>

#include "const.h"

int main() {
    auto cfg = ConfigMgr::Instance();
    std::cout << cfg["Mysql"]["Host"] << std::endl;
}
