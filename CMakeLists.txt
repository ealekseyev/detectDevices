cmake_minimum_required(VERSION 3.19)
project(detectDevices)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(detectDevices
        dhcp.h
        ethernet.h
        InetProto.h
        main.cpp
        names.h
        udp.h)