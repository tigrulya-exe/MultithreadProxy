#include "Cache.h"

bool Cache::contains(std::string& url) {
    lockMutex(&mutex, "cacheMutex", name);
    bool contains = urlToCacheNode.find(url) != urlToCacheNode.end();
    unlockMutex(&mutex, "cacheMutex", name);

    return contains;
}

int Cache::getCacheOffset(int socketFd) {
    return cacheOffsets[socketFd];
}

void Cache::setCacheOffset(int socketFd, int newOffset) {
    lockMutex(&mutex, "cacheMutex", name);
    cacheOffsets.emplace(socketFd, newOffset);
    unlockMutex(&mutex , "cacheMutex", name);
}

pthread_mutex_t& Cache::getMutex(){
    return mutex;
}

CacheNode* Cache::getCacheNode(std::string& url){
    lockMutex(&mutex, "cacheMutex", name);
    CacheNode* nodeRef = urlToCacheNode[url];
    unlockMutex(&mutex, "cacheMutex", name);
    return nodeRef;
}

void Cache::addCacheNode(std::string& path){
    lockMutex(&mutex, "cacheMutex", name);
    cacheNodes.emplace_back();
    CacheNode node = *cacheNodes.rbegin();
    urlToCacheNode.emplace(path, &(*cacheNodes.rbegin()));
    unlockMutex(&mutex,"cacheMutex", name);
}

Cache::Cache() {
    initMutex(&mutex, "cacheMutex", "cache");
}

//pthread_mutex_t& Cache::getCacheNodeMutex(std::string &URL) {
//    return cacheNodeMutexes[URL];
//}
//
//void Cache::addCacheNodeMutex(const std::string &URL, pthread_mutex_t& mutex) {
//    cacheNodeMutexes[URL] = mutex;
//}
