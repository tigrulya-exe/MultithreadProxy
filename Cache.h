#include <vector>
#include <map>

#ifndef CACHE_H
#define CACHE_H

class Cache{
private:
    std::map<std::string, bool> cacheReady;

    // key - socket fd, value - offset
    std::map<int, int> cacheOffsets;

    std::map<std::string, std::vector<char>> cacheMap;

    // key - URL, value - mutex
    std::map<std::string, pthread_mutex_t> cacheNodeMutexes;
public:
    bool contains(std::string url);

    std::vector<char>& getCurrentData(const std::string& url);

    void addData(std::string& url, char *newData, int newDataLength);

    bool cacheNodeReady(std::string& url);

    void setNodeReady(const std::string& url, bool isReady);

    int& getCacheOffset(int socketFd);

    void addCacheOffset(int socketFd, int newOffset);

    pthread_mutex_t& getCacheNodeMutex(std::string& URL);

    void addCacheNodeMutex(const std::string& URL, pthread_mutex_t& mutex);
};

#endif