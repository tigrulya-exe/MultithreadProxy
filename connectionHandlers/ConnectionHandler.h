#ifndef CONNECTIONHANDLER_H
#define CONNECTIONHANDLER_H

#include "../cache/Cache.h"

class ConnectionHandler {
    bool ready = false;

    pthread_t pthreadId;

    pthread_mutex_t readyMutex;

    bool interrupted = true;

protected:
    void setInterrupted(bool isInterrupted);

    bool isInterrupted() const;

public:
    virtual bool isServerHandlerInitiator();

    ConnectionHandler();

    bool isReady();

    virtual ~ConnectionHandler();

    void setReady();

    void setPthreadId();

    pthread_t getPthreadId();
};


#endif
