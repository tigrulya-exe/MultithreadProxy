#include "CacheNode.h"

bool CacheNode::isReady(std::string& name) {
    lockMutex(&mutex, "cacheNodeMutex", name);
    bool isReady = nodeReady;
    unlockMutex(&mutex, "cacheNodeMutex", name);

    return isReady;
}

void CacheNode::addData(char *newData, int newDataLength) {
//    lockMutex(&mutex);
    data.insert(data.end(), newData, newData + newDataLength);
//    unlockMutex(&mutex);
}

std::vector<char> CacheNode::getData(std::string& name, int offset, int length) {
    lockMutex(&mutex, "cacheNodeMutex", name);
    auto subVector = std::vector<char>(data.begin() + offset, data.begin() + offset + length);
    unlockMutex(&mutex, "cacheNodeMutex", name);

    return subVector;
}

int CacheNode::getSize(std::string& name){
    lockMutex(&mutex, "cacheNodeMutex", name);
    auto size = data.size();
    unlockMutex(&mutex, "cacheNodeMutex", name);

    return size;
}

CacheNode::CacheNode() : nodeReady(false) {
    initCondVar(&anyDataCondVar);
    initMutex(&mutex, "cacheNodeMutex", "cacheNode");
}

void CacheNode::setReady(){
    // for debug
    std::string name = "Server";
    lockMutex(&mutex, "cacheNodeMutex", name);
    nodeReady = true;
    unlockMutex(&mutex,"cacheNodeMutex", name);
}

pthread_mutex_t& CacheNode::getMutex(){
    return mutex;
}

pthread_cond_t &CacheNode::getAnyDataCondVar() {
    return anyDataCondVar;
}
