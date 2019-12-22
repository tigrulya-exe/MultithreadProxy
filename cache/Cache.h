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

    std::list<CacheNode> cacheNodes;

    std::map<std::string, CacheNode*> urlToCacheNode;

public:
    Cache();

    bool contains(std::string& url);

    CacheNode* getCacheNode(std::string &url);

    pthread_mutex_t &getMutex();

    void addCacheNode(std::string &path);

    virtual ~Cache();
};

#endif