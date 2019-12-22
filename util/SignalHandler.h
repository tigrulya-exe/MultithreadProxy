#ifndef SIGNALHANDLER_H
#define SIGNALHANDLER_H

#include <list>
#include <vector>
#include <bits/pthreadtypes.h>

class SignalHandler {
    std::vector<pthread_t>& threadIds;

    pthread_mutex_t& mutex;

    pthread_t proxyThreadId;

    void sigWait();
public:
    SignalHandler(std::vector<pthread_t> &threadIds, pthread_mutex_t &mutex, pthread_t proxyThreadId);

    static void* startThread(void*);

};

#endif