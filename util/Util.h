#ifndef MUTEXUTIL_H
#define MUTEXUTIL_H

#include <pthread.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <unistd.h>

namespace {
    void lockMutex(pthread_mutex_t *mutex, std::string&& mutexName, std::string& role) {
//        std::cout << role << " LOCK: " << mutexName << std::endl;
        if (pthread_mutex_lock(mutex)) {
            perror("Error locking mutex");
        }
    }

    void unlockMutex(pthread_mutex_t *mutex, std::string&& mutexName, std::string& role) {
//        std::cout << role << " UNLOCK: " << mutexName << std::endl;
        if (pthread_mutex_unlock(mutex)) {
            perror("Error unlocking mutex");
        }
    }

    void initMutex(pthread_mutex_t *mutex, std::string&& mutexName, std::string&& role) {
        std::cout << role << " INIT: " << mutexName << std::endl;

        if (pthread_mutex_init(mutex, nullptr) < 0) {
            perror("Error creating mutex");
            exit(EXIT_FAILURE);
        }
    }


    void initCondVar(pthread_cond_t* condVar){
        if(pthread_cond_init(condVar, NULL)){
            perror("Error creating semaphore");
            exit(EXIT_FAILURE);
        }

    }

    bool isEndOfRequest(char* buffer, int recvCount){
        return (buffer[recvCount - 1] == '\n' && buffer[recvCount - 2] == '\r' && buffer[recvCount - 3] == '\n' && buffer[recvCount - 4] == '\r');
    }

    void closeSocket(int socketFd){
        if(close(socketFd) < 0){
            perror("Error closing socket");
        }
    }

}

#endif
