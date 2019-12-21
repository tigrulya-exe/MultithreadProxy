#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include "ServerConnectionHandler.h"
#include "../constants.h"
#include "../exceptions/ProxyException.h"
#include "../httpParser/HttpParser.h"

    ServerConnectionHandler::ServerConnectionHandler(std::string &url, std::vector<char> &clientRequest, std::string &host,
                                                 Cache &cacheRef, int clientFd) : URL(url), clientRequest(clientRequest), host(host),
                                                                    cacheRef(cacheRef), clientSocketFd(clientFd) {}

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

    std::cout << "RESOLVING ADDRESS" << std::endl;
    sockaddr_in addressToConnect = getServerAddress();
    std::cout << "RESOLVED" << std::endl;

    std::cout << "CONNECTING" << std::endl;
    fcntl(serverSockFd, F_SETFL, fcntl(serverSockFd, F_GETFL, 0) | O_NONBLOCK);

    if(connect(serverSockFd, (struct sockaddr *)& addressToConnect, sizeof(addressToConnect)) == -1 && errno != EINPROGRESS){
        throw ProxyException(errors::SERVER_CONNECT);
    }

    fcntl(serverSockFd, F_SETFL,  fcntl(serverSockFd, F_GETFL, 0) & ~O_NONBLOCK);
    std::cout << "CONNECTED" << std::endl;

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
    std::cout << "SENDING: " << clientRequest.size() << std::endl;

    if (send(socketFd, clientRequest.data(), clientRequest.size(), 0) < 0) {
        throw ProxyException(errors::SERVER_CONNECT);
    }

    std::cout << "REQUEST TO " << URL << " WAS SENT" << std::endl;
}

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
        if ((recvCount = recv(socketFd, buffer, BUF_SIZE, 0)) < 0 && errno != EINPROGRESS) {
            throw ProxyException(errors::INTERNAL_ERROR);
        }

        std::cout << "SERVER GET: " <<  recvCount << "BYTES" << std::endl;

//        lockMutex(&cacheNodeMutex, "cacheNodeMutex", name);
        cacheNode->addData(buffer, recvCount);
//        unlockMutex(&cacheNodeMutex, "cacheNodeMutex", "server");

        pthread_cond_broadcast(&condVar);
    }

    cacheNode->setReady();
    pthread_cond_broadcast(&condVar);

    std::cout << "GET RESPONSE" << std::endl;
}

bool ServerConnectionHandler::isCorrectResponseStatus(char *response, int responseLength) {
    size_t bodyLen;
    const char *body;
    int version, status = 0;
    struct phr_header headers[100];
    size_t num_headers = sizeof(headers) / sizeof(headers[0]);

    phr_parse_response(response, responseLength, &version, &status, &body, &bodyLen, headers, &num_headers, 0);

    return status == 200;
}

void ServerConnectionHandler::handle() {
    try {
        std::cout << "START SERVER THREAD" << std::endl;
        socketFd = initServerConnection();

        sendRequestToServer();

        getResponseFromServer();
        closeSocket(socketFd);
    }
    catch (ProxyException& exception){
        sendError(exception.what(), clientSocketFd);
    }
    catch (std::exception& exception){
        std::cerr << exception.what() << std::endl;
    }
}