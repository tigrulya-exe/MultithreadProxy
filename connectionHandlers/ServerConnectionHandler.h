#ifndef SERVERCONNECTIONHANDLER_H
#define SERVERCONNECTIONHANDLER_H

#include <string>
#include <vector>
#include "../cache/Cache.h"
#include "ConnectionHandler.h"

class ServerConnectionHandler : public ConnectionHandler {
    std::string& URL;

    std::vector<char>& clientRequest;

    std::string& host;

    Cache& cacheRef;

    int socketFd;

    int clientSocketFd;

    void handle();

    int initServerConnection();

    bool isCorrectResponseStatus(char *response, int responseLength);

    void sendRequestToServer();

    void getResponseFromServer();

    struct sockaddr_in getServerAddress();

public:

    ServerConnectionHandler(std::string &url, std::vector<char> &clientRequest, std::string &host, Cache &cacheRef, int clientFd);

    static void* startThread(void* );

    virtual ~ServerConnectionHandler() override;

    void handleError(const char *what);
};

#endif