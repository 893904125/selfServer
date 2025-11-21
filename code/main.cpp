#include <iostream>

#include "./log/log.h"

int main() {
    Log::Instance()->init();
    LOG_INFO("Hello World!");

}