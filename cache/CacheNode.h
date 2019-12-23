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

    bool isReady();

    void addData(char *newData, int newDataLength);

    int getSize();

    std::vector<char> getData(int offset, int length);

    void setReady();

    pthread_mutex_t &getMutex();

    pthread_cond_t &getAnyDataCondVar();

    virtual ~CacheNode();

    int getSizeWithoutLock();

    bool isReadyWithoutLock();
};


#endif
