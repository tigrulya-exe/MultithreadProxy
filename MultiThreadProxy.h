
#include "constants.h"
#include "cache/Cache.h"
#include "httpParser/HttpRequest.h"
#include "connectionHandlers/ClientConnectionHandler.h"
#include <vector>
#include <memory>

#ifndef MULTYTHREADPROXY_H
#define MULTYTHREADPROXY_H

class MultiThreadProxy {
    static const int MAX_CONNECTIONS = 510;

    int acceptSocketFd;

    int portToListen;

    Cache cache;

    std::list<std::shared_ptr<ClientConnectionHandler>> connectionHandlers;

public:
    explicit MultiThreadProxy(int portToListen);

    void start();

    int initAcceptSocket();

    void initAddress(struct sockaddr_in *addr, int port);

    void addNewConnection(int newSocketFd);

    void joinThreads();

    void checkConnectionHandlers();
};


#endif
