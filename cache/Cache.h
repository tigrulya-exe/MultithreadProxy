#include <vector>
#include <map>
#include <list>
#include <memory>
#include "CacheNode.h"

#ifndef CACHE_H
#define CACHE_H

class Cache{
private:
    pthread_mutex_t mutex;

    std::map<std::string, CacheNode* > urlToCacheNode;

public:
    Cache();

    bool contains(std::string& url);

    CacheNode * getCacheNode(std::string &url);

    void addCacheNode(std::string &path);

    void removeCacheNode(std::string &path);

    virtual ~Cache();
};

#endif