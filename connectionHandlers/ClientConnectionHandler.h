#ifndef CLIENT_CONNECTIONHANDLER_H
#define CLIENT_CONNECTIONHANDLER_H

#include <string>
#include <vector>
#include "../cache/Cache.h"
#include "../httpParser/HttpRequest.h"
#include "ConnectionHandler.h"
#include "ServerConnectionHandler.h"

class ClientConnectionHandler : public ConnectionHandler {
    std::shared_ptr<ServerConnectionHandler> serverConnectionHandler = nullptr;

    std::string URL;

    std::vector<char> clientRequest;

    Cache& cacheRef;

    std::vector<pthread_t>& threadIds;

    pthread_mutex_t& threadIdsMutex;

    int socketFd;

    HttpRequest parseHttpRequest(std::vector<char>& request, std::string& newRequest);

    void handle();

    void getDataFromClient();

    void checkUrl(HttpRequest &request);

    void sendDataFromCache();

    void initServerThread(std::string &host);

    void checkRequest(HttpRequest &request);

    void handleClientRequest(HttpRequest& request);

public:
    explicit ClientConnectionHandler(int socketFd, Cache& cache, std::vector<pthread_t>& threadIds, pthread_mutex_t& threadIdsMutex);

    static void* startThread(void* );

    virtual bool isServerAvailable() override ;

    std::shared_ptr<ConnectionHandler> getServerConnectionHandler();

    virtual ~ClientConnectionHandler() override;
};

#endif
