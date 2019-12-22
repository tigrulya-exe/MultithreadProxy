#ifndef CONNECTIONHANDLER_H
#define CONNECTIONHANDLER_H

#include <string>
#include <vector>
#include "../cache/Cache.h"
#include "../httpParser/HttpRequest.h"

class ClientConnectionHandler {
    bool ready = false;

    pthread_mutex_t readyMutex;

    std::string URL;

    std::vector<char> clientRequest;

    Cache& cacheRef;

    HttpRequest parseHttpRequest(std::vector<char>& request, std::string& newRequest);

    int socketFd;

    void handle();

    void getDataFromClient();

    void checkUrl(HttpRequest &request);

    void sendDataFromCache();

    void initServerThread(std::string &host);

public:
    explicit ClientConnectionHandler(int socketFd, Cache& cache);

    static void* startThread(void* );

    bool isReady();

    const std::string &getUrl() const;

    void checkRequest(HttpRequest &request);

    virtual ~ClientConnectionHandler();

    void setReady();
};

#endif
