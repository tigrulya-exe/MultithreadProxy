
#include "constants.h"
#include "cache/Cache.h"
#include "httpParser/HttpRequest.h"
#include "connectionHandlers/ClientConnectionHandler.h"
#include "util/SignalHandler.h"
#include <vector>
#include <memory>

#ifndef MULTYTHREADPROXY_H
#define MULTYTHREADPROXY_H


class MultiThreadProxy {
    static const int MAX_CONNECTIONS = 512;

    static bool isInterrupted;

    SignalHandler signalHandler;

    Cache cache;

    std::list<std::shared_ptr<ClientConnectionHandler>> connectionHandlers;

    std::vector<pthread_t> threadIds;

    pthread_mutex_t threadIdsMutex;

    int acceptSocketFd;

    int portToListen;

    int initAcceptSocket();

    void initAddress(struct sockaddr_in *addr, int port);

    void addNewConnection(int newSocketFd);

    void checkConnectionHandlers();

    bool readyToConnect();

    void initSignalHandlerThread();

    void setSigUsrHandler();

    void joinThreads();

    static void interrupt (int sig);
public:

    explicit MultiThreadProxy(int portToListen);

    void start();

    virtual ~MultiThreadProxy();
};


#endif
