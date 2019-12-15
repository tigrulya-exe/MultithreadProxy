#include <netinet/in.h>
#include <iostream>
#include <netdb.h>
#include <unistd.h>
#include <semaphore.h>
#include "MultiThreadProxy.h"
#include "exceptions/ProxyException.h"
#include "exceptions/SocketClosedException.h"
#include "models/HttpRequest.h"
#include "models/ServerConnection.h"

MultiThreadProxy::MultiThreadProxy(int portToListen) : portToListen(portToListen) {}

// todo add mutex
std::vector<pthread_t> threadIds;

bool isInterrupted = false;

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
    }

    joinThreads();
}

void semPost(sem_t* sem){
    if(sem_post(sem) < 0){
        perror("Error posting to semaphore");
    }
}

void semWait(sem_t* sem){
    if(sem_wait(sem) < 0){
        perror("Error waiting at semaphore");
    }
}


// TODO tmp
void lockMutex(pthread_mutex_t* mutex){
    if(pthread_mutex_lock(mutex)){
        perror("Error locking mutex");
    }
}

// TODO tmp
void unlockMutex(pthread_mutex_t* mutex){
    if(pthread_mutex_unlock(mutex)){
        perror("Error unlocking mutex");
    }
}

void closeSocket(int socketFd){
    if(close(socketFd) < 0){
        perror("Error closing socket");
    }
}


bool isEndOfRequest(char* buffer, int recvCount){
    return (buffer[recvCount - 1] == '\n' && buffer[recvCount - 2] == '\r' && buffer[recvCount - 3] == '\n' && buffer[recvCount - 4] == '\r');
}

void getDataFromClient(Connection& connection) {
    char buffer[BUF_SIZE];
    int recvCount = 0;

    do{
        if ((recvCount = recv(connection.socketFd, buffer, BUF_SIZE, 0)) < 0) {
            throw ProxyException(errors::INTERNAL_ERROR);
        }

        if(recvCount == 0)
            throw SocketClosedException();

        connection.clientRequest.insert(connection.clientRequest.end(), buffer, buffer + recvCount);
    }while (!isEndOfRequest(buffer, recvCount));
}

HttpRequest parseHttpRequest(std::vector<char>& request){
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

    for(int i = 0; i < httpRequest.headersCount; ++i){
        std::string headerName = httpRequest.headers[i].name;
        headerName.erase(headerName.begin() + httpRequest.headers[i].name_len, headerName.end());

        if(headerName == "Host") {
            std::string headerValue = httpRequest.headers[i].value;
            headerValue.erase(headerValue.begin() + httpRequest.headers[i].value_len, headerValue.end());
            httpRequest.host = headerValue;
        }
    }

    return httpRequest;
}

void sendDataFromCache(Connection &connection) {
    Cache& cache = connection.cacheRef;
    std::string& URL = connection.URL;

    auto& cacheNode = cache.getCurrentData(connection.URL);
    int& offset = cache.getCacheOffset(connection.socketFd);
    auto& cacheNodeMutex = cache.getCacheNodeMutex(connection.URL);

    int sendCount;
    int size = 0;

    lockMutex(&cacheNodeMutex);

    while(!cache.cacheNodeReady(URL) || offset != (size = cacheNode.size())) {
        unlockMutex(&cacheNodeMutex);

        if ((sendCount = send(connection.socketFd, cacheNode.data() + offset, size - offset, 0)) < 0) {
            perror("Error sending data to client from cache");
            throw ProxyException(errors::CACHE_SEND_ERROR);
        }


//        if (sendCount) {
            std::cout << connection.socketFd << " : GOT DATA ( " << sendCount << " BYTES) FROM CACHE:" << connection.URL
                      << std::endl;
//        }

        offset += sendCount;

        if(!cache.cacheNodeReady(URL))
            semWait(&connection.semaphore);
        lockMutex(&cacheNodeMutex);
    }

    unlockMutex(&cacheNodeMutex);

    semPost(&connection.semaphore);

}

sockaddr_in getServerAddress(const char *host) {
    sockaddr_in serverAddr;

    addrinfo addrInfo = {0};
    addrInfo.ai_flags = 0;
    addrInfo.ai_family = AF_INET;
    addrInfo.ai_socktype = SOCK_STREAM;
    addrInfo.ai_protocol = IPPROTO_TCP;

    addrinfo* hostAddr = nullptr;
    getaddrinfo(host, nullptr, &addrInfo, &hostAddr);

    if (!hostAddr) {
        std::cout << "Can't resolve host!" << std::endl;
        throw ProxyException(errors::BAD_REQUEST);
    }

    serverAddr = *(sockaddr_in*) (hostAddr->ai_addr);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(80);

    return serverAddr;
}

void addCacheNodeMutex(std::string& URL, Cache& cache) {
    pthread_mutex_t mutex;

    if(pthread_mutex_init(&mutex, NULL) < 0){
        perror("Error creating mutex");
        exit(EXIT_FAILURE);
    }

    cache.addCacheNodeMutex(URL, mutex);
}

int initServerConnection(std::string& serverHostName){
    int serverSockFd = socket(AF_INET, SOCK_STREAM, 0);

    if(serverSockFd < 0){
        throw ProxyException(errors::SERVER_CONNECT);
    }

    sockaddr_in addressToConnect = getServerAddress(serverHostName.c_str());

    if(connect(serverSockFd, (struct sockaddr *)& addressToConnect, sizeof(addressToConnect)) == -1){
        throw ProxyException(errors::SERVER_CONNECT);
    }

    return serverSockFd;
}

void sendRequestToServer(ServerConnection& serverConnection){
    if(send(serverConnection.socketFd, serverConnection.clientRequest.data(), serverConnection.clientRequest.size(), 0) < 0){
        throw ProxyException(errors::SERVER_CONNECT);
    }

    std::cout << "REQUEST TO SERVER WAS SENT" << std::endl;
}

void getResponseFromServer(ServerConnection& serverConnection){
    int recvCount = -1;
    char buffer[BUF_SIZE];
    Cache& cache = serverConnection.cacheRef;
//    addCacheNodeMutex(serverConnection)
    pthread_mutex_t cacheNodeMutex = cache.getCacheNodeMutex(serverConnection.URL);


    while (recvCount != 0){
        if ((recvCount = recv(serverConnection.socketFd, buffer, BUF_SIZE, 0)) < 0) {
            throw ProxyException(errors::INTERNAL_ERROR);
        }

        lockMutex(&cacheNodeMutex);
        std::cout << "SERVER GET: " <<  recvCount << "BYTES" << std::endl;
        cache.addData(serverConnection.URL, buffer, recvCount);
        semPost(&serverConnection.semaphore);

        unlockMutex(&cacheNodeMutex);
    }

    lockMutex(&cacheNodeMutex);

    cache.setNodeReady(serverConnection.URL, true);

    unlockMutex(&cacheNodeMutex);

    std::cout << "GET RESPONSE" << std::endl;

}


void* handleServerConnection(void* serverConnectionIn){
    ServerConnection* serverConnection = (ServerConnection *) serverConnectionIn;

    try {
        serverConnection->socketFd = initServerConnection(serverConnection->host);
        sendRequestToServer(*serverConnection);
        getResponseFromServer(*serverConnection);
        closeSocket(serverConnection->socketFd);

        delete (serverConnection);
    }
    catch (std::exception& exception){
        std::cerr << exception.what() << std::endl;
    }

    return nullptr;
}

void initServerThread(Connection& connection, std::string& host) {
    pthread_t newThreadId = 0;

    addCacheNodeMutex(connection.URL, connection.cacheRef);

    if(sem_init(&connection.semaphore, 0, 0)){
        perror("Error creating semaphore");
        exit(EXIT_FAILURE);
    }

    auto* serverConnection = new ServerConnection(host, connection.URL, connection.clientRequest,
            connection.cacheRef,connection.semaphore);
    if (pthread_create(&newThreadId, NULL, handleServerConnection, (void *) (serverConnection))) {
        perror("Error creating thread");
    }

    threadIds.push_back(newThreadId);
}

void checkUrl(Connection& connection, HttpRequest& request){
    Cache& cache = connection.cacheRef;
    if(!cache.contains(connection.URL))
        initServerThread(connection, request.host);
}

void* handleClientConnection(void* connectionIn){
    Connection connection = *(Connection *)connectionIn;

    try {
        getDataFromClient(connection);
        HttpRequest request = parseHttpRequest(connection.clientRequest);
        connection.URL = request.path;
        checkUrl(connection, request);
        sendDataFromCache(connection);
        closeSocket(connection.socketFd);
    } catch (std::exception& exception){
        std::cerr << exception.what() << std::endl;
    }


    return nullptr;
}

void MultiThreadProxy::joinThreads(){
    for(auto& threadId : threadIds){
        if (pthread_join(threadId,NULL)){
            perror("Error waiting thread");
        }
    }
}

void MultiThreadProxy::addNewConnection(int newSocketFd){
    connections.emplace_back(newSocketFd, cache);
    pthread_t newthreadId = 0;

    if (pthread_create(&newthreadId, NULL, handleClientConnection, (void *) (&connections.back()))) {
        perror("Error creating thread");
    }

    threadIds.push_back(newthreadId);
}

int MultiThreadProxy::initAcceptSocket(){
    sockaddr_in address;
    initAddress(&address, portToListen);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
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