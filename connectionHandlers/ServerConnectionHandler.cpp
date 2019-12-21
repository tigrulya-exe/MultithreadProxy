//
// Created by tigrulya on 12/21/19.
//

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "ServerConnectionHandler.h"
#include "../constants.h"
#include "../exceptions/ProxyException.h"

ServerConnectionHandler::ServerConnectionHandler(std::string &url, std::vector<char> &clientRequest, std::string &host,
                                                 Cache &cacheRef) : URL(url), clientRequest(clientRequest), host(host),
                                                                    cacheRef(cacheRef) {}

void *ServerConnectionHandler::startThread(void * handler) {
    auto* serverConnectionHandler = (ServerConnectionHandler *) handler;
    serverConnectionHandler->handle();
    delete (serverConnectionHandler);
    return nullptr;
}


int ServerConnectionHandler::initServerConnection(){
    int serverSockFd = socket(AF_INET, SOCK_STREAM, 0);

    if(serverSockFd < 0){
        throw ProxyException(errors::SERVER_CONNECT);
    }

    sockaddr_in addressToConnect = getServerAddress();

    if(connect(serverSockFd, (struct sockaddr *)& addressToConnect, sizeof(addressToConnect)) == -1){
        throw ProxyException(errors::SERVER_CONNECT);
    }

    return serverSockFd;
}

sockaddr_in ServerConnectionHandler::getServerAddress() {
    sockaddr_in serverAddr;

    addrinfo addrInfo = {0};
    addrInfo.ai_flags = 0;
    addrInfo.ai_family = AF_INET;
    addrInfo.ai_socktype = SOCK_STREAM;
    addrInfo.ai_protocol = IPPROTO_TCP;

    addrinfo* hostAddr = nullptr;
    getaddrinfo(host.c_str(), nullptr, &addrInfo, &hostAddr);

    if (!hostAddr) {
        std::cout << "Can't resolve host!" << std::endl;
        throw ProxyException(errors::BAD_REQUEST);
    }

    serverAddr = *(sockaddr_in*) (hostAddr->ai_addr);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(80);

    return serverAddr;
}

void ServerConnectionHandler::sendRequestToServer(){
    if(send(socketFd, clientRequest.data(), clientRequest.size(), 0) < 0){
        throw ProxyException(errors::SERVER_CONNECT);
    }

    std::cout << "REQUEST TO SERVER WAS SENT" << std::endl;
}

//void ServerConnectionHandler::getResponseFromServer(){
//    int recvCount = -1;
//    char buffer[BUF_SIZE];
//
//    //for debug
//    std::string name = "Server";
//
//    // tmp add mutexes
//    CacheNode& cacheNode = cacheRef.getCacheNode(URL);
//    pthread_mutex_t& cacheNodeMutex = cacheNode.getMutex();
//    auto& condVar = cacheNode.getAnyDataCondVar();
//
//
//    while (recvCount != 0){
//        if ((recvCount = recv(socketFd, buffer, BUF_SIZE, 0)) < 0) {
//            throw ProxyException(errors::INTERNAL_ERROR);
//        }
//
//        std::cout << "SERVER GET: " <<  recvCount << "BYTES" << std::endl;
//
////        lockMutex(&cacheNodeMutex, "cacheNodeMutex", name);
//        cacheNode.addData(buffer, recvCount);
////        unlockMutex(&cacheNodeMutex, "cacheNodeMutex", "server");
//
//        pthread_cond_broadcast(&condVar);
////        unlockMutex(&cacheNodeMutex, "cacheNodeMutex", "server");
//    }
//
//    cacheNode.setReady();
//
//    pthread_cond_broadcast(&condVar);
//
//    std::cout << "GET RESPONSE" << std::endl;
//}

void ServerConnectionHandler::getResponseFromServer(){
    int recvCount = -1;
    char buffer[BUF_SIZE];

    //for debug
    std::string name = "Server";

    // tmp add mutexes
    CacheNode* cacheNode = cacheRef.getCacheNode(URL);
    pthread_mutex_t& cacheNodeMutex = cacheNode->getMutex();
    auto& condVar = cacheNode->getAnyDataCondVar();


    while (recvCount != 0){
        if ((recvCount = recv(socketFd, buffer, BUF_SIZE, 0)) < 0) {
            throw ProxyException(errors::INTERNAL_ERROR);
        }

        std::cout << "SERVER GET: " <<  recvCount << "BYTES" << std::endl;

//        lockMutex(&cacheNodeMutex, "cacheNodeMutex", name);
        cacheNode->addData(buffer, recvCount);
//        unlockMutex(&cacheNodeMutex, "cacheNodeMutex", "server");

        pthread_cond_broadcast(&condVar);
//        unlockMutex(&cacheNodeMutex, "cacheNodeMutex", "server");
    }

    cacheNode->setReady();

    pthread_cond_broadcast(&condVar);

    std::cout << "GET RESPONSE" << std::endl;
}


void ServerConnectionHandler::handle() {
    try {
        socketFd = initServerConnection();
        sendRequestToServer();
        getResponseFromServer();
        closeSocket(socketFd);

    }
    catch (std::exception& exception){
        std::cerr << "HERE" << exception.what() << std::endl;
    }
}