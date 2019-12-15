
#include "constants.h"
#include "Cache.h"
#include "models/Connection.h"
#include <vector>

#ifndef MULTYTHREADPROXY_H
#define MULTYTHREADPROXY_H

class MultiThreadProxy {
    static const int ACCEPT_INDEX = 0;
    static const int MAX_CONNECTIONS = 510;

    int acceptSocketFd;

    int portToListen;

    Cache cache;

    std::vector<Connection> connections;
public:
    explicit MultiThreadProxy(int portToListen);

    void start();

    int initAcceptSocket();

    void initAddress(struct sockaddr_in *addr, int port);

    void addNewConnection(int newSocketFd);

    void joinThreads();

};


#endif
