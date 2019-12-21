#ifndef SERVERCONNECTIONHANDLER_H
#define SERVERCONNECTIONHANDLER_H

#include <string>
#include <vector>
#include "../cache/Cache.h"

class ServerConnectionHandler {
    std::string& URL;

    std::vector<char>& clientRequest;

    std::string& host;

    Cache& cacheRef;

    int socketFd;

    void handle();
public:
    ServerConnectionHandler(std::string &url, std::vector<char> &clientRequest, std::string &host, Cache &cacheRef);

    static void* startThread(void* );

    int initServerConnection();

    struct sockaddr_in getServerAddress();

    void sendRequestToServer();

    void getResponseFromServer();
};

#endif