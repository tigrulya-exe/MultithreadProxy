//
// Created by tigrulya on 12/14/19.
//
#ifndef CONNECTION_H
#define CONNECTION_H

#include <string>
#include <vector>
#include <semaphore.h>
#include "../cache/Cache.h"

struct Connection {
    explicit Connection(int socketFd) : socketFd(socketFd) {}

    int socketFd;

    std::string URL;

//    std::vector<char> clientRequest;

//    Cache& cacheRef;

//    pthread_cond_t anyDataCondVar;
};


#endif