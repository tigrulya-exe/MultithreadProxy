#include "Cache.h"

bool Cache::contains(std::string url) {
    return cacheMap.find(url) != cacheMap.end();
}

std::vector<char> &Cache::getCurrentData(const std::string& url) {
    return cacheMap[url];
}

void Cache::addData(std::string& url, char *newData, int newDataLength) {
    auto& cacheNode = cacheMap[url];
    cacheNode.insert(cacheNode.end(), newData, newData + newDataLength);
}

bool Cache::cacheNodeReady(std::string& url) {
    return !contains(url) ? false : cacheReady[url];
}

void Cache::setNodeReady(const std::string& url, bool isReady) {
    cacheReady[url] = isReady;
}

int& Cache::getCacheOffset(int socketFd) {
    return cacheOffsets[socketFd];
}

void Cache::addCacheOffset(int socketFd, int newOffset) {
    cacheOffsets[socketFd] = newOffset;
}

pthread_mutex_t& Cache::getCacheNodeMutex(std::string &URL) {
    return cacheNodeMutexes[URL];
}

void Cache::addCacheNodeMutex(const std::string &URL, pthread_mutex_t& mutex) {
    cacheNodeMutexes[URL] = mutex;
}
