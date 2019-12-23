#include <signal.h>
#include <cstdio>
#include <cstdlib>
#include "SignalHandler.h"
#include "Util.h"

SignalHandler::SignalHandler(std::vector<pthread_t> &threadIds, pthread_mutex_t &mutex, pthread_t proxyThreadId)
                                                : threadIds(threadIds), mutex(mutex), proxyThreadId(proxyThreadId) {}

void *SignalHandler::startThread(void* signalHandlerPtr) {
    SignalHandler* signalHandler = (SignalHandler *)signalHandlerPtr;
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

    int sig;

    if(sigwait(&sigSet, &sig) < 0){
        perror("Error waiting for signal");
        exit(EXIT_FAILURE);
    }

    std::cout << "SIGNAL RECEIVED" << std::endl;


    lockMutex(&mutex);
    for(auto& threadId : threadIds){
        if(pthread_cancel(threadId) < 0){
            perror("ERROR CANCELING");
        }
        std::cout << "SIGKILL WAS SENT TO" << threadId << std::endl;
        fflush(stdout);
    }
    unlockMutex(&mutex);

    pthread_kill(proxyThreadId, SIGUSR1);


    std::cout << "SIGNAL END" << std::endl;
    fflush(stdout);

}
