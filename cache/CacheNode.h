#ifndef CACHENODE_H
#define CACHENODE_H

#include <pthread.h>
#include <vector>
#include <cstdio>
#include "../util/Util.h"

class CacheNode {
    bool nodeReady;

    pthread_mutex_t mutex;

    std::vector<char> data;

    pthread_cond_t anyDataCondVar;

public:

    CacheNode();

    void addData(const char *newData, int newDataLength);

    std::vector<char> getData(int offset, int length);

    void setReady();

    pthread_mutex_t &getMutex();

    pthread_cond_t &getAnyDataCondVar();

    virtual ~CacheNode();

    int getSizeWithoutLock();

    bool isReadyWithoutLock();
};


#endif
