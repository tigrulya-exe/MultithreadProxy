#include <netinet/in.h>
#include <unistd.h>
#include <semaphore.h>
#include <csignal>
#include <algorithm>
#include "MultiThreadProxy.h"
#include "exceptions/ProxyException.h"
#include "exceptions/SocketClosedException.h"
#include "httpParser/HttpRequest.h"
#include "util/SignalHandler.h"

bool MultiThreadProxy::isInterrupted = false;

MultiThreadProxy::MultiThreadProxy(int portToListen) :
    portToListen(portToListen), signalHandler(threadIds, threadIdsMutex, pthread_self()) {
    initMutex(&threadIdsMutex);
    setSigMask();
    setSigUsrHandler();
    initSignalHandlerThread();
}

void MultiThreadProxy::setSigUsrHandler(){
    struct sigaction sigAction;

    sigAction.sa_handler = interrupt;
    sigemptyset (&sigAction.sa_mask);
    sigAction.sa_flags = 0;

    sigaction (SIGUSR1, &sigAction, nullptr);

    sigAction.sa_handler = SIG_IGN;
    sigaction (SIGPIPE, &sigAction, nullptr);
}

void MultiThreadProxy::start(){
    acceptSocketFd = initAcceptSocket();
    int newSocketFd = 0;

    while (!isInterrupted){
        newSocketFd = accept(acceptSocketFd, nullptr, nullptr);

        if(newSocketFd < 0){
            perror("Error accepting connection");
            continue;
        }

        addNewConnection(newSocketFd);
        checkConnectionHandlers();
    }
}

void MultiThreadProxy::addNewConnection(int newSocketFd){
    if(!readyToConnect()){
        closeSocket(newSocketFd);
        return;
    }

    auto&& ptr = std::make_shared<ClientConnectionHandler>(newSocketFd, cache, threadIds, threadIdsMutex);

    connectionHandlers.emplace_back(ptr);
    pthread_t newThreadId = 0;

    pthread_attr_t detachedAttr;
    setDetachedAttribute(&detachedAttr);

    if (pthread_create(&newThreadId, &detachedAttr, ClientConnectionHandler::startThread, (void *) (connectionHandlers.back().get()))){
        perror("Error creating thread");
        connectionHandlers.remove(connectionHandlers.back());
        return;
    }

    lockMutex(&threadIdsMutex);
    threadIds.push_back(newThreadId);
    unlockMutex(&threadIdsMutex);
}

bool MultiThreadProxy::readyToConnect(){
    lockMutex(&threadIdsMutex);
    int currentThreads = threadIds.size();
    unlockMutex(&threadIdsMutex);

    return currentThreads <= MAX_CONNECTIONS;
}

void MultiThreadProxy::initSignalHandlerThread(){
    pthread_t newThreadId;

    if (pthread_create(&newThreadId, NULL, SignalHandler::startThread, (void *) &signalHandler)){
        perror("Error creating signal handler thread");
        exit(EXIT_FAILURE);
    }
}

int MultiThreadProxy::initAcceptSocket(){
    int reuse = 1;
    sockaddr_in address;
    initAddress(&address, portToListen);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    if(bind(sockfd, (struct sockaddr *)&address, sizeof(address)) < 0){
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if(listen(sockfd, MAX_CONNECTIONS) < 0){
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

void MultiThreadProxy::initAddress(sockaddr_in* addr, int port){
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = INADDR_ANY;
    addr->sin_port = htons(port);
}

void MultiThreadProxy::removeThreadId(pthread_t threadIdToRemove){
    lockMutex(&threadIdsMutex);
    threadIds.erase(std::remove_if(threadIds.begin(), threadIds.end(), [&threadIdToRemove](const pthread_t& threadId) {
        return threadId == threadIdToRemove;
    }), threadIds.end());
    unlockMutex(&threadIdsMutex);
}

bool MultiThreadProxy::checkIsServerReady(std::_List_iterator<std::shared_ptr<ConnectionHandler>>& connectionsHandler){
    if((*connectionsHandler)->isServerHandlerInitiator()){
        auto* connectionHandler = dynamic_cast<ClientConnectionHandler*> (connectionsHandler->get());
        std::shared_ptr<ConnectionHandler>&& serverConnectionHandler = connectionHandler->getServerConnectionHandler();
        if(!serverConnectionHandler->isReady())
            connectionHandlers.push_back(serverConnectionHandler);
        else
            removeThreadId(serverConnectionHandler->getPthreadId());
    }
}

void MultiThreadProxy::checkConnectionHandlers() {
    for (auto iter = connectionHandlers.begin(); iter != connectionHandlers.end(); ) {
        if ((*iter) ->isReady()) {
            removeThreadId((*iter)->getPthreadId());
            checkIsServerReady(iter);
            iter = connectionHandlers.erase(iter);
            continue;
        }
        iter++;
    }
}

MultiThreadProxy::~MultiThreadProxy() {
    destroyMutex(&threadIdsMutex);
    connectionHandlers.clear();
}

void MultiThreadProxy::interrupt(int sig) {
    isInterrupted = true;
}