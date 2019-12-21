#ifndef CONNECTIONHANDLER_H
#define CONNECTIONHANDLER_H

#include <string>
#include <vector>
#include "../cache/Cache.h"
#include "../httpParser/HttpRequest.h"
#include "../models/Connection.h"

class ConnectionHandler {
    bool ready = false;

    std::string URL;

    std::vector<char> clientRequest;

    Cache& cacheRef;

    HttpRequest parseHttpRequest(std::vector<char>& request, std::string& newRequest);

    Connection connection;

    void handle();

    void getDataFromClient();

    void checkUrl(HttpRequest &request);

    void sendDataFromCache();

    void initServerThread(std::string &host);

public:
    explicit ConnectionHandler(Connection&& connection, Cache& cache);

    explicit ConnectionHandler(int socketFd, Cache& cache);

    static void* startThread(void* );

    bool isReady() const;

    const Connection &getConnection() const;
};


#endif //MULTYTHREADPROXY_CONNECTIONHANDLER_H
