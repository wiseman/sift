#include "Mutex.hpp"


Mutex::Mutex()
{
    pthread_mutex_init(&m_mutex, NULL);
}

Mutex::~Mutex()
{
    pthread_mutex_destroy(&m_mutex);
}

bool Mutex::acquire()
{
    return (pthread_mutex_lock(&m_mutex) == 0);
}

bool Mutex::release()
{
    return (pthread_mutex_unlock(&m_mutex) == 0);
}


AcquirableLock::AcquirableLock(Mutex& mutex)
    : m_mutex(&mutex)
{
    m_mutex->acquire();
}

AcquirableLock::~AcquirableLock()
{
    m_mutex->release();
}

