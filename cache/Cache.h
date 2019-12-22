#include <vector>
#include <map>
#include <list>
#include <memory>
#include "CacheNode.h"

#ifndef CACHE_H
#define CACHE_H

class Cache{
private:
    // for debug
    std::string name = "Cache";

    pthread_mutex_t mutex;

    std::map<std::string, std::shared_ptr<CacheNode>> urlToCacheNode;
public:
    Cache();

    bool contains(std::string& url);

    std::shared_ptr<CacheNode>& getCacheNode(std::string &url);

    void addCacheNode(std::string &path);

    virtual ~Cache();
};

#endif