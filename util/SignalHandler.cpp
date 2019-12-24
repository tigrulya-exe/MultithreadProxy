#include <signal.h>
#include <cstdio>
#include <cstdlib>
#include "SignalHandler.h"
#include "Util.h"

SignalHandler::SignalHandler(std::vector<pthread_t> &threadIds, pthread_mutex_t &mutex, pthread_t proxyThreadId)
                                                : threadIds(threadIds), mutex(mutex), proxyThreadId(proxyThreadId) {}

void *SignalHandler::startThread(void* signalHandlerPtr) {
    auto* signalHandler = (SignalHandler *)signalHandlerPtr;
    signalHandler->sigWait();
    return nullptr;
}

void SignalHandler::sigWait() {
    setSigMask();
    sigset_t sigSet = getSigMask();

    if(pthread_sigmask(SIG_BLOCK, &sigSet, NULL)){
        perror("Error setting signal mask");
        exit(EXIT_FAILURE);
    }

    int signal;

    if(sigwait(&sigSet, &signal) < 0){
        perror("Error waiting for signal");
        exit(EXIT_FAILURE);
    }

    std::cout << "SIGNAL RECEIVED: " << signal << std::endl;

    lockMutex(&mutex);
    for(auto& threadId : threadIds){
        if(pthread_cancel(threadId) < 0){
            perror("Error canceling thread");
        }

#ifdef DEBUG_CANCEL_THREAD_LIST
        std::cout << threadId << " : was cancelled" << std::endl;
        fflush(stdout);
#endif

    }
    unlockMutex(&mutex);

    pthread_kill(proxyThreadId, SIGUSR1);

    std::cout << "SIGNAL HANDLER EXIT" << std::endl;
    fflush(stdout);
}
