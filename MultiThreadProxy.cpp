#include <netinet/in.h>
#include <unistd.h>
#include <semaphore.h>
#include <csignal>
#include "MultiThreadProxy.h"
#include "exceptions/ProxyException.h"
#include "exceptions/SocketClosedException.h"
#include "httpParser/HttpRequest.h"
#include "util/SignalHandler.h"

bool MultiThreadProxy::isInterrupted = false;

MultiThreadProxy::MultiThreadProxy(int portToListen) :
    portToListen(portToListen), signalHandler(threadIds, threadIdsMutex) {
    initMutex(&threadIdsMutex);
    setSigMask();
    setSigUsrHandler();
    initSignalHandlerThread();
}

// todo add correct error handling

void MultiThreadProxy::setSigUsrHandler(){
    struct sigaction sigAction;

    sigAction.sa_handler = interrupt;
    sigemptyset (&sigAction.sa_mask);
    sigAction.sa_flags = 0;

    sigaction (SIGUSR1, &sigAction, nullptr);
}


void MultiThreadProxy::start(){
    signal(SIGPIPE, SIG_IGN);
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

    joinThreads();
}

void MultiThreadProxy::joinThreads(){
    for(auto& threadId : threadIds){
        if (pthread_join(threadId,NULL)){
            perror("Error waiting thread");
        }
    }
}

void MultiThreadProxy::addNewConnection(int newSocketFd){
    if(!readyToConnect()){
        closeSocket(newSocketFd);
        return;
    }

    auto&& ptr = std::make_shared<ClientConnectionHandler>(newSocketFd, cache);

    connectionHandlers.emplace_back(ptr);
    pthread_t newThreadId = 0;

    if (pthread_create(&newThreadId, NULL, ClientConnectionHandler::startThread, (void *) (connectionHandlers.back().get()))){
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

    lockMutex(&threadIdsMutex);
    threadIds.push_back(newThreadId);
    unlockMutex(&threadIdsMutex);
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

void MultiThreadProxy::checkConnectionHandlers() {
    for (auto iter = connectionHandlers.begin();iter != connectionHandlers.end(); ) {
        if ((*iter) ->isReady()) {
            std::cout << "REMOVED: " << (*iter) ->getUrl() << std::endl;
            iter = connectionHandlers.erase(iter);
            continue;
        }
        iter++;
    }
}

MultiThreadProxy::~MultiThreadProxy() {
    destroyMutex(&threadIdsMutex);
}

void MultiThreadProxy::interrupt(int sig) {
    isInterrupted = true;
}