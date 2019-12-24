
#include "ConnectionHandler.h"

ConnectionHandler::ConnectionHandler() {
    initMutex(&readyMutex);
}

void ConnectionHandler::setReady(){
    lockMutex(&readyMutex);
    ready = true;
    unlockMutex(&readyMutex);
}

ConnectionHandler::~ConnectionHandler() {
    destroyMutex(&readyMutex);
}

bool ConnectionHandler::isReady() {
    lockMutex(&readyMutex);
    bool isReady = ready;
    unlockMutex(&readyMutex);

    return isReady;
}

pthread_t ConnectionHandler::getPthreadId() {
    return pthreadId;
}

void ConnectionHandler::setInterrupted(bool isInterrupted) {
    interrupted = isInterrupted;
}

bool ConnectionHandler::isInterrupted() const {
    return interrupted;
}

bool ConnectionHandler::isServerHandlerInitiator() {
    return false;
}

void ConnectionHandler::setPthreadId() {
    pthreadId = pthread_self();
}
