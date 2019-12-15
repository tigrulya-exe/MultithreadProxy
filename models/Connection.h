//
// Created by tigrulya on 12/14/19.
//
#ifndef CONNECTION_H
#define CONNECTION_H

#include <string>
#include <vector>
#include <semaphore.h>
#include "../Cache.h"

struct Connection {
    explicit Connection(int socketFd, Cache& cacheRef) : socketFd(socketFd), cacheRef(cacheRef) {}

    int socketFd;

    std::string URL;

    std::vector<char> clientRequest;

    Cache& cacheRef;

    sem_t semaphore;
};


#endif