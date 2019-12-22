#include <signal.h>
#include <cstdio>
#include <cstdlib>
#include "SignalHandler.h"
#include "Util.h"

SignalHandler::SignalHandler(std::vector<pthread_t> &threadIds, pthread_mutex_t &mutex) : threadIds(threadIds),
                                                                                    mutex(mutex) {}

void *SignalHandler::startThread(void* signalHandlerPtr) {
    SignalHandler* signalHandler = (SignalHandler *)signalHandlerPtr;
    signalHandler->sigWait();
    return nullptr;
}

void SignalHandler::sigWait() {
    sigset_t sigSet = getSigMask();

    if(pthread_sigmask(SIG_BLOCK, &sigSet, NULL)){
        perror("Error setting signal mask");
        exit(EXIT_FAILURE);
    }

    int sig;
    if(sigwait(&sigSet, &sig) < 0){
        perror("Error waiting for signal");
        exit(EXIT_FAILURE);
    }

    for(auto& threadId : threadIds){
        pthread_kill(threadId, SIGUSR1);
    }
}
