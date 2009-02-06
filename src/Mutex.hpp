#ifndef MUTEX_HPP
#define MUTEX_HPP

#include <pthread.h>

class Mutex
{
public:
    Mutex();
    ~Mutex();

public:
    bool acquire();
    bool release();
    
protected:
    pthread_mutex_t m_mutex;
};

class AcquirableLock
{
public:
    AcquirableLock(Mutex& mutex);
    ~AcquirableLock();

protected:
    Mutex *m_mutex;
};



#endif
