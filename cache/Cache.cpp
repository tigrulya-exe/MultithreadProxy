#include "Cache.h"

bool Cache::contains(std::string& url) {
    lockMutex(&mutex, "cacheMutex", name);
    bool contains = urlToCacheNode.find(url) != urlToCacheNode.end();
    unlockMutex(&mutex, "cacheMutex", name);

    return contains;
}

std::shared_ptr<CacheNode>& Cache::getCacheNode(std::string& url){
    lockMutex(&mutex, "cacheMutex", name);
    auto& nodeRef = urlToCacheNode[url];
    unlockMutex(&mutex, "cacheMutex", name);
    return nodeRef;
}

void Cache::addCacheNode(std::string& path){
    lockMutex(&mutex, "cacheMutex", name);
    urlToCacheNode.emplace(path, std::make_shared<CacheNode>());
    unlockMutex(&mutex,"cacheMutex", name);
}

Cache::Cache() {
    initMutex(&mutex);
}

Cache::~Cache() {
    destroyMutex(&mutex);
}