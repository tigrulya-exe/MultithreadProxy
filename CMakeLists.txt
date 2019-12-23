cmake_minimum_required(VERSION 3.15)
project(multythreadProxy)

find_package(Threads REQUIRED)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -lrt")


add_executable(multythreadProxy main.cpp cache/Cache.cpp cache/Cache.h constants.h httpParser/HttpParser.c  httpParser/HttpParser.h exceptions/ProxyException.h exceptions/SocketClosedException.h exceptions/WrongArgumentException.h httpParser/HttpRequest.h util/ArgResolver.cpp util/ArgResolver.h MultiThreadProxy.cpp MultiThreadProxy.h cache/CacheNode.cpp cache/CacheNode.h util/Util.h connectionHandlers/ClientConnectionHandler.cpp connectionHandlers/ClientConnectionHandler.h connectionHandlers/ServerConnectionHandler.cpp connectionHandlers/ServerConnectionHandler.h util/SignalHandler.cpp util/SignalHandler.h connectionHandlers/ConnectionHandler.h connectionHandlers/ConnectionHandler.cpp)