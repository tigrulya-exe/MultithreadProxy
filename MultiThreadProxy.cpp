#include <netinet/in.h>
#include <unistd.h>
#include <semaphore.h>
#include <csignal>
#include "MultiThreadProxy.h"
#include "exceptions/ProxyException.h"
#include "exceptions/SocketClosedException.h"
#include "httpParser/HttpRequest.h"

MultiThreadProxy::MultiThreadProxy(int portToListen) : portToListen(portToListen) {}

// todo add mutex
std::vector<pthread_t> threadIds;
// todo remove conditional variable after (or before?) serverSocket death
// todo add correct error handling

bool isInterrupted = false;

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
    auto&& ptr = std::make_shared<ConnectionHandler>(newSocketFd, cache);

    connectionHandlers.emplace_back(ptr);
    pthread_t newThreadId = 0;

    if (pthread_create(&newThreadId, NULL, ConnectionHandler::startThread, (void *) (connectionHandlers.back().get()))){
        perror("Error creating thread");
    }

    threadIds.push_back(newThreadId);
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