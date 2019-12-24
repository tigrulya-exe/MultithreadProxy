#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include "ServerConnectionHandler.h"
#include "../constants.h"
#include "../exceptions/ProxyException.h"
#include "../httpParser/HttpParser.h"

ServerConnectionHandler::ServerConnectionHandler(std::string &url, std::vector<char> &clientRequest, std::string &host,
                                                 Cache &cacheRef, int clientFd) : ConnectionHandler(), URL(url), clientRequest(clientRequest), host(host),
                                                                    cacheRef(cacheRef), clientSocketFd(clientFd) {}

void *ServerConnectionHandler::startThread(void * handler) {
    auto* serverConnectionHandler = (ServerConnectionHandler *) handler;

    serverConnectionHandler->setPthreadId();
#ifdef DEBUG
    pthread_t pthreadId = serverConnectionHandler->getPthreadId();
    std::cout << "START SERVER THREAD: " << pthreadId  << std::endl;
#endif
    serverConnectionHandler->handle();

#ifdef DEBUG
//    std::cout << "STOP SERVER THREAD: " << pthreadId << std::endl;
#endif

    return nullptr;
}

int ServerConnectionHandler::initServerConnection(){
    int serverSockFd = socket(AF_INET, SOCK_STREAM, 0);

    if(serverSockFd < 0){
        throw ProxyException(errors::SERVER_CONNECT);
    }

#ifdef DEBUG
    std::cout << "RESOLVING ADDRESS" << std::endl;
#endif

    sockaddr_in addressToConnect = getServerAddress();

#ifdef DEBUG
    std::cout << "RESOLVED\nCONNECTING" << std::endl;
#endif

    fcntl(serverSockFd, F_SETFL, fcntl(serverSockFd, F_GETFL, 0) | O_NONBLOCK);

    if(connect(serverSockFd, (struct sockaddr *)& addressToConnect, sizeof(addressToConnect)) == -1 && errno != EINPROGRESS){
        throw ProxyException(errors::SERVER_CONNECT);
    }

    fcntl(serverSockFd, F_SETFL,  fcntl(serverSockFd, F_GETFL, 0) & ~O_NONBLOCK);

#ifdef DEBUG
    std::cout << "CONNECTED" << std::endl;
#endif

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

    if (send(socketFd, clientRequest.data(), clientRequest.size(), 0) < 0) {
        throw ProxyException(errors::SERVER_CONNECT);
    }

#ifdef DEBUG
    std::cout << "REQUEST TO " << URL << " WAS SENT" << std::endl;
#endif

}

void ServerConnectionHandler::getResponseFromServer(){
    int recvCount = -1;
    char buffer[BUF_SIZE];

    auto& cacheNode = cacheRef.getCacheNode(URL);
    auto& condVar = cacheNode->getAnyDataCondVar();

    int len = 0;
    while (recvCount != 0){
        if ((recvCount = recv(socketFd, buffer, BUF_SIZE, 0)) < 0) {
            throw ProxyException(errors::INTERNAL_ERROR);
        }

        cacheNode->addData(buffer, recvCount);
        pthread_cond_broadcast(&condVar);
        len += recvCount;
    }

    cacheNode->setReady();
    pthread_cond_broadcast(&condVar);

//    isCorrectResponseStatus(cacheNode->getData(0,len).data(), len);
#ifdef DEBUG
    std::cout << "FULL RESPONSE" << std::endl;
#endif

}

bool ServerConnectionHandler::isCorrectResponseStatus(char *response, int responseLength) {
    size_t bodyLen;
    const char *body;
    int version, status = 0;
    struct phr_header headers[100];
    size_t num_headers = sizeof(headers) / sizeof(headers[0]);

    phr_parse_response(response, responseLength, &version, &status, &body, &bodyLen, headers, &num_headers, 0);

    std::cout << "STATUS" << status << std::endl;
    return status == 200;
}

void ServerConnectionHandler::handle() {
    try {
        setSigMask();
        socketFd = initServerConnection();

        sendRequestToServer();

        getResponseFromServer();
        closeSocket(socketFd);
        setInterrupted(false);
    }
    catch (ProxyException& exception){
        handleError(exception.what());
    }
    catch (std::exception& exception){
        std::cerr << exception.what() << std::endl;
    }

    setReady();
}

ServerConnectionHandler::~ServerConnectionHandler() {
    if(isInterrupted())
        closeSocket(socketFd);
}

void ServerConnectionHandler::handleError(const char* error) {
    auto& cacheNode = cacheRef.getCacheNode(URL);
    cacheNode->setReady();
    cacheNode->addData(error, strlen(error));
    pthread_cond_broadcast(&cacheNode->getAnyDataCondVar());
}