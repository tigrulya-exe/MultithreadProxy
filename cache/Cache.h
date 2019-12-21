#include <vector>
#include <map>
#include <list>
#include "CacheNode.h"

#ifndef CACHE_H
#define CACHE_H

class Cache{
private:
    using iterator = std::list<CacheNode>::reverse_iterator;

    // for debug
    std::string name = "Cache";

    pthread_mutex_t mutex;
    // key - socket fd, value - offset
    std::map<int, int> cacheOffsets;

    std::list<CacheNode> cacheNodes;

    std::map<std::string, CacheNode*> urlToCacheNode;

public:
    Cache();

    bool contains(std::string& url);

    int getCacheOffset(int socketFd);

    void setCacheOffset(int socketFd, int newOffset);

    CacheNode* getCacheNode(std::string &url);

    pthread_mutex_t &getMutex();

    void addCacheNode(std::string &path);
};

#endif