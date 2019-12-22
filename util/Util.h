#ifndef MUTEXUTIL_H
#define MUTEXUTIL_H

#include <pthread.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <signal.h>

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

    void lockMutex(pthread_mutex_t *mutex) {
        if (pthread_mutex_lock(mutex)) {
            perror("Error locking mutex");
        }
    }

    void unlockMutex(pthread_mutex_t *mutex) {
//        std::cout << role << " UNLOCK: " << mutexName << std::endl;
        if (pthread_mutex_unlock(mutex)) {
            perror("Error unlocking mutex");
        }
    }

    void initMutex(pthread_mutex_t *mutex) {
        if (pthread_mutex_init(mutex, nullptr) < 0) {
            perror("Error creating mutex");
            exit(EXIT_FAILURE);
        }
    }

    void destroyMutex(pthread_mutex_t *mutex){
        if (pthread_mutex_destroy(mutex) < 0) {
            perror("Error destroying mutex");
        }
    }

    void destroyCondVar(pthread_cond_t* condVar){
        if (pthread_cond_destroy(condVar) < 0) {
            perror("Error destroying conditional variable");
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

    void sendError(const char *what, int socketFd) {
        if (send(socketFd, what, strlen(what), 0) < 0) {
            perror("Error sending error response to client");
        }
        std::cout << "ERROR RESPONSE WAS SENT TO: " << socketFd << std::endl;
    }


    void sigAddSet(sigset_t* sigSet, int sig){
        if (sigaddset(sigSet, sig) < 0){
            perror("Error setting signal mask");
            exit(EXIT_FAILURE);
        }
    }

    sigset_t getSigMask(){
        sigset_t sigSet;

        if(sigemptyset(&sigSet) < 0){
            perror("Error setting signal mask");
            exit(EXIT_FAILURE);
        }

        sigAddSet(&sigSet, SIGINT);
        sigAddSet(&sigSet, SIGTERM);
        sigAddSet(&sigSet, SIGQUIT);

        return sigSet;
    }

    void setSigMask(){
        sigset_t sigSet = getSigMask();

        if(pthread_sigmask(SIG_BLOCK, &sigSet, NULL) < 0){
            perror("Error setting signal mask");
            exit(EXIT_FAILURE);
        }
    }
}

#endif