#ifndef SIGNALHANDLER_H
#define SIGNALHANDLER_H

#include <list>
#include <vector>
#include <bits/pthreadtypes.h>

class SignalHandler {
    std::vector<pthread_t>& threadIds;

    pthread_mutex_t& mutex;

    void sigWait();
public:
    SignalHandler(std::vector<pthread_t> &threadIds, pthread_mutex_t &mutex);

    static void* startThread(void*);

};

#endif