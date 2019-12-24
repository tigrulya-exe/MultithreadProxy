#include <sys/socket.h>
#include <cstring>
#include "ClientConnectionHandler.h"
#include "../constants.h"
#include "../exceptions/ProxyException.h"
#include "../exceptions/SocketClosedException.h"
#include "ServerConnectionHandler.h"

//CC -lrt -lxnet -lnsl cache/*.cpp connectionHandlers/*.cpp util/*.cpp
// httpParser/*.c  *.cpp -std=c++11  -lgcc_s -lc -lpthread -m32  -o proxy -std=c++11

void *ClientConnectionHandler::startThread(void * connectionHandlerIn) {
    auto* handler = (ClientConnectionHandler *)connectionHandlerIn;
    handler->setPthreadId();
    handler->handle();
    return NULL;
}

HttpRequest ClientConnectionHandler::parseHttpRequest(std::vector<char>& request, std::string& newRequest){
    HttpRequest httpRequest;

    const char* path;
    const char* method;
    int version;
    size_t methodLen, pathLen;

    httpRequest.headersCount = sizeof(httpRequest.headers) / sizeof(httpRequest.headers[0]);
    int reqSize = phr_parse_request(request.data(), request.size(), &method, &methodLen,
                                    &path, &pathLen, &version, httpRequest.headers,
                                    &httpRequest.headersCount, 0);

    if(reqSize == -1){
        throw ProxyException(errors::BAD_REQUEST);
    }

    std::string onlyPath = path;
    onlyPath.erase(onlyPath.begin() + pathLen, onlyPath.end());
    httpRequest.path = onlyPath;

    std::string onlyMethod = method;
    onlyMethod.erase(onlyMethod.begin() + methodLen, onlyMethod.end());
    httpRequest.method = onlyMethod;

    newRequest = onlyMethod + std::string(" ").append(onlyPath).append(" HTTP/1.0") + "\r\n";


    for (int i = 0; i < httpRequest.headersCount; ++i) {
        std::string headerName = httpRequest.headers[i].name;
        headerName.erase(headerName.begin() + httpRequest.headers[i].name_len, headerName.end());
        std::string headerValue = httpRequest.headers[i].value;
        headerValue.erase(headerValue.begin() + httpRequest.headers[i].value_len, headerValue.end());

        if(headerName == "Connection")
            continue;

        if (headerName == "Host") {
            httpRequest.host = headerValue;
        }

        newRequest.append(headerName).append(": ").append(headerValue) += "\r\n";
    }

    newRequest += "\r\n\r\n";

    return httpRequest;
}

void ClientConnectionHandler::handleClientRequest(HttpRequest& request){
    getDataFromClient();

    std::string newRequest;
    request = parseHttpRequest(clientRequest, newRequest);
    checkRequest(request);

    clientRequest.clear();
    clientRequest = std::vector<char >(newRequest.begin(), newRequest.end());

    URL = request.path;
    checkUrl(request);
}

void ClientConnectionHandler::handle() {
    try {
        setSigMask();

        HttpRequest request;
        handleClientRequest(request);

#ifdef DEBUG
        std::cout << "REQUEST HANDLED: " << getPthreadId() << std::endl;
#endif

        sendDataFromCache();

        std::cout << "[" << socketFd <<  "] finish: " << URL << std::endl;

    } catch (ProxyException& exception){
        sendError(exception.what(), socketFd);
    }
    catch (std::exception& exception){
        std::cerr << exception.what() << std::endl;
    }

    closeSocket(socketFd);
    setInterrupted(false);
    setReady();
}

bool ClientConnectionHandler::isServerHandlerInitiator() {
    return serverConnectionHandler.get() != NULL;
}

void ClientConnectionHandler::checkRequest(HttpRequest& request){
    if(request.method == "GET" && request.method == "HEAD"){
        throw ProxyException(errors::NOT_IMPLEMENTED);
    }
}

void ClientConnectionHandler::getDataFromClient() {
    char buffer[BUF_SIZE];
    int recvCount = 0;

    do{
        if ((recvCount = recv(socketFd, buffer, BUF_SIZE, 0)) < 0) {
            throw ProxyException(errors::INTERNAL_ERROR);
        }

        if(recvCount == 0)
            break;

        clientRequest.insert(clientRequest.end(), buffer, buffer + recvCount);
    }while (!isEndOfRequest(buffer, recvCount));
}

void ClientConnectionHandler::checkUrl(HttpRequest& request){
    if(!cacheRef.contains(URL)) {
        cacheRef.addCacheNode(URL);
        initServerThread(request.host);
    }
}

void ClientConnectionHandler::initServerThread(std::string& host) {
    pthread_t newThreadId = 0;

    serverConnectionHandler = std::make_shared<ServerConnectionHandler>( URL, clientRequest, host, cacheRef, socketFd);
    pthread_attr_t detachedAttr;
    setDetachedAttribute(&detachedAttr);

    if (pthread_create(&newThreadId, &detachedAttr, ServerConnectionHandler::startThread, (void *) (serverConnectionHandler.get()))) {
        perror("Error creating thread");
    }

    lockMutex(&threadIdsMutex);
    threadIds.push_back(newThreadId);
    unlockMutex(&threadIdsMutex);
}


void ClientConnectionHandler::sendDataFromCache() {
    int sendCount, offset = 0, size = 0;
    bool cacheNodeReady = false;

    auto* cacheNode = cacheRef.getCacheNode(URL);
    auto& cacheNodeMutex = cacheNode->getMutex();
    auto& condVar = cacheNode->getAnyDataCondVar();

    cacheNode->addListener();

    while (!cacheNodeReady || offset != size) {

        lockMutex(&cacheNodeMutex);
        while (offset == (size = cacheNode->getSizeWithoutLock()) &&
                !(cacheNodeReady = (cacheNode->isReadyWithoutLock()))) {
            std::cout << "COND_WAIT: " << getPthreadId() << std::endl;
            fflush(stdout);
            pthread_cond_wait(&condVar, &cacheNodeMutex);
            std::cout << "COND_RELS: " << getPthreadId() << std::endl;

        }
        unlockMutex(&cacheNodeMutex);

        if ((sendCount = send(socketFd,
                              cacheNode->getData(offset, size - offset).data(), size - offset, 0)) < 0) {
            perror("Error sending data to client from cache");
            cacheNode->removeListener();
            throw ProxyException(errors::CACHE_SEND_ERROR);
        }

#ifdef DEBUG_GOT_DATA
        std::cout << socketFd << " : GOT DATA ( " << sendCount << " BYTES) FROM CACHE:" << URL
                  << std::endl;
#endif

        offset += sendCount;
    }

    cacheNode->removeListener();
}

ClientConnectionHandler::ClientConnectionHandler(int socketFd, Cache &cache, std::vector<pthread_t>& threadIds, pthread_mutex_t& threadIdsMutex)
                                                    : cacheRef(cache), socketFd(socketFd), threadIds(threadIds), threadIdsMutex(threadIdsMutex) {
}

ClientConnectionHandler::~ClientConnectionHandler() {
    if(isInterrupted())
        closeSocket(socketFd);
}

std::shared_ptr<ConnectionHandler> ClientConnectionHandler::getServerConnectionHandler() {
    return std::shared_ptr<ConnectionHandler>(serverConnectionHandler);
}