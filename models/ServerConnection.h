//
// Created by tigrulya on 12/14/19.
//

#ifndef SERVERCONNECTION_H
#define SERVERCONNECTION_H


#include <string>
#include <vector>
#include "../Cache.h"

struct ServerConnection {
    ServerConnection(std::string &host, std::string &url, std::vector<char> &clientRequest, Cache &cacheRef, sem_t& semaphore) : host(
            host), URL(url), clientRequest(clientRequest), cacheRef(cacheRef), semaphore(semaphore) {}

    int socketFd;

    std::string& URL;

    std::string& host;

    std::vector<char>& clientRequest;

    Cache& cacheRef;

    sem_t& semaphore;
};


#endif
