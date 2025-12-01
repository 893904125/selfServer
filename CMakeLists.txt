cmake_minimum_required(VERSION 3.10)
project(selfServer)

# 1. 设置 C++ 标准 (C++14 足够，也更稳定)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 开启调试信息和所有警告
set(CMAKE_BUILD_TYPE "Debug")
add_compile_options(-Wall)

# 2. 查找依赖库
# (A) 查找 Threads (Log系统必须用!)
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads REQUIRED)

# (B) 查找 Boost (既然你之前加了，我就保留它，虽然你的代码看起来主要用标准库)
find_package(Boost REQUIRED COMPONENTS filesystem system)
include_directories(${Boost_INCLUDE_DIRS})

# 3. 设置头文件搜索路径
# 这样你在代码里 #include "log/log.h" 或 #include "log.h" 都能找到
include_directories(
        code/server
        code/log
        code/buffer
        code/config
        code/pool
        code
)

# 4. 生成可执行文件
# 注意：我帮你去掉了重复的文件，并修正了 potential 的文件名问题
add_executable(selfServer
        code/main.cpp
        code/log/log.cpp
        code/log/log.h
        code/buffer/buffer.cpp
        code/buffer/buffer.h
        code/log/BlockDeque.h

        code/pool/SqlConnPool.cpp
        code/pool/SqlConnPool.h
        code/server/epoller.cpp
        code/server/epoller.h
        code/timer/heaptimer.cpp
        code/timer/heaptimer.h
        code/config/ConfigMgr.cpp
        code/config/ConfigMgr.h
        code/server/webserver.cpp
        code/server/webserver.h
        code/pool/ThreadPool.h
        code/http/httpconn.cpp
        code/http/httpconn.h
        code/http/httprequest.cpp
        code/http/httprequest.h
        code/http/httpresponse.cpp
        code/http/httpresponse.h
)

# 5. 链接库
target_link_libraries(selfServer
        Threads::Threads      # 必须链接线程库
        Boost::system
        Boost::filesystem
        mysqlclient
        pthread
        ${Boost_LIBRARIES}    # 链接 Boost
)

# 这里的 config.ini 是源路径，${CMAKE_BINARY_DIR}/config.ini 是目标路径
file(COPY ${CMAKE_SOURCE_DIR}/config.ini
        DESTINATION ${CMAKE_BINARY_DIR})