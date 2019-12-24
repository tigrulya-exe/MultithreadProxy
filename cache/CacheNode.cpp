#include "CacheNode.h"

bool CacheNode::isReadyWithoutLock() {
    return nodeReady;
}

void CacheNode::addData(const char *newData, int newDataLength) {
    lockMutex(&mutex);
    data.insert(data.end(), newData, newData + newDataLength);
    unlockMutex(&mutex);
}

std::vector<char> CacheNode::getData(int offset, int length) {
    lockMutex(&mutex);
    auto subVector = std::vector<char>(data.begin() + offset, data.begin() + offset + length);
    unlockMutex(&mutex);

    return subVector;
}

int CacheNode::getSizeWithoutLock(){
    return data.size();
}

CacheNode::CacheNode() : nodeReady(false) {
    initCondVar(&anyDataCondVar);
    initMutex(&mutex);
}

void CacheNode::setReady(){
    lockMutex(&mutex);
    nodeReady = true;
    unlockMutex(&mutex);
}

pthread_mutex_t& CacheNode::getMutex(){
    return mutex;
}

pthread_cond_t &CacheNode::getAnyDataCondVar() {
    return anyDataCondVar;
}

CacheNode::~CacheNode() {
    destroyMutex(&mutex);
    if(!condVarDestroyed){
        destroyCondVar(&anyDataCondVar);
    }
}

void CacheNode::removeListener() {
    lockMutex(&mutex);
    --listenersCount;

    if(!condVarDestroyed && listenersCount == 0 && nodeReady){
        condVarDestroyed = true;
        destroyCondVar(&anyDataCondVar);
        std::cout << "CV DESTROYED" << std::endl;
        fflush(stdout);
    }

    unlockMutex(&mutex);
}

void CacheNode::addListener() {
    lockMutex(&mutex);
    ++listenersCount;
    unlockMutex(&mutex);
}

