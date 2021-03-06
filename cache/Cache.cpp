#include "Cache.h"

bool Cache::contains(std::string& url) {
    lockMutex(&mutex);
    bool contains = urlToCacheNode.find(url) != urlToCacheNode.end();
    unlockMutex(&mutex);

    return contains;
}
//
//std::shared_ptr<CacheNode>& Cache::getCacheNode(std::string& url){
//    lockMutex(&mutex);
//    auto* nodeRef = urlToCacheNode[url];
//    unlockMutex(&mutex);
//    return nodeRef;
//}


CacheNode* Cache::getCacheNode(std::string& url){
    lockMutex(&mutex);
    auto* nodeRef = urlToCacheNode[url];
    unlockMutex(&mutex);
    return nodeRef;
}


void Cache::addCacheNode(std::string& path){
    lockMutex(&mutex);
    urlToCacheNode.emplace(path, new CacheNode());
    unlockMutex(&mutex);
}

Cache::Cache() {
    initMutex(&mutex);
}

Cache::~Cache() {
    for(auto& cacheNode : urlToCacheNode){
        delete(cacheNode.second);
    }

    destroyMutex(&mutex);
}

void Cache::removeCacheNode(std::string &path) {
    lockMutex(&mutex);
    urlToCacheNode.erase(path);
    unlockMutex(&mutex);
}
