#include <sys/socket.h>
#include "ConnectionHandler.h"
#include "../constants.h"
#include "../exceptions/ProxyException.h"
#include "../models/Connection.h"
#include "../exceptions/SocketClosedException.h"
#include "../models/ServerConnection.h"
#include "ServerConnectionHandler.h"

void *ConnectionHandler::startThread(void * connectionHandlerIn) {
    auto* handler = (ConnectionHandler *)connectionHandlerIn;
    handler->handle();
    return nullptr;
}

HttpRequest ConnectionHandler::parseHttpRequest(std::vector<char>& request, std::string& newRequest){
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
//
//        if(headerName != "")
//            continue;

        newRequest.append(headerName).append(": ").append(headerValue) += "\r\n";
    }

    newRequest += "\r\n\r\n";

    return httpRequest;
}

void ConnectionHandler::handle() {

    try {
        getDataFromClient();
        std::string newRequest;
        HttpRequest request = parseHttpRequest(clientRequest, newRequest);
        clientRequest.clear();
        clientRequest = std::vector<char >(newRequest.begin(), newRequest.end());
        connection.URL = request.path;

        checkUrl(request);
        sendDataFromCache();
        closeSocket(connection.socketFd);
        std::cout << connection.socketFd <<  " finish: " << connection.URL << std::endl;
    } catch (std::exception& exception){
        std::cerr << exception.what() << std::endl;
    }

    ready = true;
}

void ConnectionHandler::getDataFromClient() {
    char buffer[BUF_SIZE];
    int recvCount = 0;

    do{
        if ((recvCount = recv(connection.socketFd, buffer, BUF_SIZE, 0)) < 0) {
            throw ProxyException(errors::INTERNAL_ERROR);
        }

        if(recvCount == 0)
            throw SocketClosedException();

        clientRequest.insert(clientRequest.end(), buffer, buffer + recvCount);
    }while (!isEndOfRequest(buffer, recvCount));
}

void ConnectionHandler::checkUrl(HttpRequest& request){
    if(!cacheRef.contains(connection.URL)) {
        cacheRef.addCacheNode(connection.URL);
        initServerThread(request.host);
    }
}

void ConnectionHandler::initServerThread(std::string& host) {
    pthread_t newThreadId = 0;

//    addCacheNodeMutex(connection.URL, connection.cacheRef);

    auto* serverConnectionHandler = new ServerConnectionHandler( connection.URL, clientRequest, host, cacheRef);
    if (pthread_create(&newThreadId, NULL, ServerConnectionHandler::startThread, (void *) (serverConnectionHandler))) {
        perror("Error creating thread");
    }

//    threadIds.push_back(newThreadId);
}

//void ConnectionHandler::sendDataFromCache() {
////    int offset = cacheRef.getCacheOffset(connection.socketFd);
//    int offset = 0;
//
//    int sendCount;
//    int size = 0;
//
//    auto* cacheNode = cacheRef.getCacheNode(connection.URL);
//    auto &cacheNodeMutex = cacheNode.getMutex();
//    auto& condVar = cacheNode.getAnyDataCondVar();
//
//    //for debug
//    std::string name = "Client";
//
//    while (!cacheNode.isReady(name) || offset != cacheNode.getSize(name)) {
//
//        if(cacheNode )
//
//        while (offset == (size = cacheNode.getSize(name))) {
//            lockMutex(&cacheNodeMutex, "cacheNodeMutex", name);
//            pthread_cond_wait(&condVar, &cacheNodeMutex);
//            unlockMutex(&cacheNodeMutex, "cacheNodeMutex", name);
//        }
//
////        if ((sendCount = send(connection.socketFd, data.data() + offset, size - offset, 0)) < 0) {
//        if ((sendCount = send(connection.socketFd, cacheNode.getData(name, offset, size - offset).data(),
//                size - offset, 0)) < 0) {
//
//                perror("Error sending data to client from cache");
//            throw ProxyException(errors::CACHE_SEND_ERROR);
//        }
//
//        std::cout << connection.socketFd << " : GOT DATA ( " << sendCount << " BYTES) FROM CACHE:" << connection.URL
//                  << std::endl;
////        }
//
////        cacheRef.setCacheOffset(connection.socketFd, offset += sendCount);
//
//        offset += sendCount;
////        lockMutex(&cacheNodeMutex);
////        cacheNode = cache.getCurrentData(connection.URL);
//
//        // tmp
//    }
//}

void ConnectionHandler::sendDataFromCache() {
//    int offset = cacheRef.getCacheOffset(connection.socketFd);
    int offset = 0;

    int sendCount;
    int size = 0;

    auto* cacheNode = cacheRef.getCacheNode(connection.URL);
    auto &cacheNodeMutex = cacheNode->getMutex();
    auto& condVar = cacheNode->getAnyDataCondVar();

    //for debug
    std::string name = "Client";

    if(cacheNode == NULL){
        std::cout << "NULL" << std::endl;
    }
    while (!cacheNode->isReady(name) || offset != cacheNode->getSize(name)) {

            while (offset == (size = cacheNode->getSize(name))) {
                lockMutex(&cacheNodeMutex, "cacheNodeMutex", name);
                pthread_cond_wait(&condVar, &cacheNodeMutex);
                unlockMutex(&cacheNodeMutex, "cacheNodeMutex", name);
            }

//        if ((sendCount = send(connection.socketFd, data.data() + offset, size - offset, 0)) < 0) {
        if ((sendCount = send(connection.socketFd, cacheNode->getData(name, offset, size - offset).data(),
                              size - offset, 0)) < 0) {

            perror("Error sending data to client from cache");
            throw ProxyException(errors::CACHE_SEND_ERROR);
        }

        std::cout << connection.socketFd << " : GOT DATA ( " << sendCount << " BYTES) FROM CACHE:" << connection.URL
                  << std::endl;
//        }

//        cacheRef.setCacheOffset(connection.socketFd, offset += sendCount);

        offset += sendCount;
//        lockMutex(&cacheNodeMutex);
//        cacheNode = cache.getCurrentData(connection.URL);

        // tmp
    }
}

ConnectionHandler::ConnectionHandler(Connection &&connection, Cache& cache) : connection(connection) ,cacheRef(cache){
}

bool ConnectionHandler::isReady() const {
    return ready;
}

const Connection &ConnectionHandler::getConnection() const {
    return connection;
}

ConnectionHandler::ConnectionHandler(int socketFd, Cache &cache) : cacheRef(cache) {

}
