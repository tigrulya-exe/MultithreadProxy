cmake_minimum_required(VERSION 3.15)
project(multythreadProxy)

find_package(Threads REQUIRED)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -lrt")


add_executable(multythreadProxy main.cpp Cache.cpp Cache.h constants.h httpParser/HttpParser.c  httpParser/HttpParser.h exceptions/ProxyException.h exceptions/SocketClosedException.h exceptions/WrongArgumentException.h models/HttpRequest.h util/ArgResolver.cpp util/ArgResolver.h MultiThreadProxy.cpp MultiThreadProxy.h models/Connection.h models/ServerConnection.h)