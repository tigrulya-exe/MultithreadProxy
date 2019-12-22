#include "Cache.h"

bool Cache::contains(std::string& url) {
    lockMutex(&mutex, "cacheMutex", name);
    bool contains = urlToCacheNode.find(url) != urlToCacheNode.end();
    unlockMutex(&mutex, "cacheMutex", name);

    return contains;
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
    urlToCacheNode.emplace(path, &(*cacheNodes.rbegin()));
    unlockMutex(&mutex,"cacheMutex", name);
}

Cache::Cache() {
    initMutex(&mutex);
}

Cache::~Cache() {
    destroyMutex(&mutex);
}
