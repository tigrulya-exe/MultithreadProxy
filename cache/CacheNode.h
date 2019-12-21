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

    bool isReady(std::string& name);

    void addData(char *newData, int newDataLength);

    int getSize(std::string& name);

    std::vector<char> getData(std::string& name, int offset, int length);

    void setReady();

    pthread_mutex_t &getMutex();

    pthread_cond_t &getAnyDataCondVar();
};


#endif
