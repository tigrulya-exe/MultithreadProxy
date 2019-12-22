#include "CacheNode.h"

bool CacheNode::isReady() {
    lockMutex(&mutex);
    bool isReady = nodeReady;
    unlockMutex(&mutex);

    return isReady;
}

void CacheNode::addData(char *newData, int newDataLength) {
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

int CacheNode::getSize(){
    lockMutex(&mutex);
    auto size = data.size();
    unlockMutex(&mutex);

    return size;
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
    destroyCondVar(&anyDataCondVar);
}