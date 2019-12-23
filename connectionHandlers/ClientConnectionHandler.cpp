#include <sys/socket.h>
#include <cstring>
#include "ClientConnectionHandler.h"
#include "../constants.h"
#include "../exceptions/ProxyException.h"
#include "../exceptions/SocketClosedException.h"
#include "ServerConnectionHandler.h"

void *ClientConnectionHandler::startThread(void * connectionHandlerIn) {
    auto* handler = (ClientConnectionHandler *)connectionHandlerIn;
    handler->setPthreadId();
    handler->handle();
    return nullptr;
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

        std::cout << "REQUEST HANDLED: " << getPthreadId() << std::endl;

        sendDataFromCache();

        std::cout << socketFd <<  " finish: " << URL << std::endl;

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

bool ClientConnectionHandler::isServerAvailable() {
    return serverConnectionHandler != nullptr;
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

//        if(recvCount == 0)
//            throw SocketClosedException();

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

    if (pthread_create(&newThreadId, NULL, ServerConnectionHandler::startThread, (void *) (serverConnectionHandler.get()))) {
        perror("Error creating thread");
    }

    lockMutex(&threadIdsMutex);
    threadIds.push_back(newThreadId);
    unlockMutex(&threadIdsMutex);
}


void ClientConnectionHandler::sendDataFromCache() {
    int sendCount, offset = 0, size = 0;
    bool cacheNodeReady = false;

    auto& cacheNode = cacheRef.getCacheNode(URL);
    auto& cacheNodeMutex = cacheNode->getMutex();
    auto& condVar = cacheNode->getAnyDataCondVar();

    //for debug
    std::string name = "Client";

    while (!cacheNodeReady || offset != size) {

        lockMutex(&cacheNodeMutex, "cacheNodeMutex", name);
        while (offset == (size = cacheNode->getSizeWithoutLock()) &&
                !(cacheNodeReady = (cacheNode->isReadyWithoutLock()))) {
            pthread_cond_wait(&condVar, &cacheNodeMutex);
        }
        unlockMutex(&cacheNodeMutex, "cacheNodeMutex", name);

        if ((sendCount = send(socketFd,
                              cacheNode->getData(offset, size - offset).data(), size - offset, 0)) < 0) {
            perror("Error sending data to client from cache");
            throw ProxyException(errors::CACHE_SEND_ERROR);
        }

//        std::cout << socketFd << " : GOT DATA ( " << sendCount << " BYTES) FROM CACHE:" << URL
//                  << std::endl;

        offset += sendCount;
    }
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