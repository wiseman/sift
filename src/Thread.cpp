#include "Thread.hpp"

#include <assert.h>


Thread::Thread(ThreadProc proc, ThreadProcParam param, bool is_detached)
  : m_thread(0), m_is_detached(is_detached), m_proc(proc), m_param(param)
{
}

Thread::~Thread()
{
}

bool Thread::start()
{
    pthread_attr_t attr;
    if (pthread_attr_init(&attr) != 0) {
        return false;
    }
    if (m_is_detached) {
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    }
    if (pthread_create(&m_thread, NULL, launch_thread_proc, this) != 0) {
        pthread_attr_destroy(&attr);
        return false;
    }

    return true;
}

bool Thread::stop()
{
  assert(false);
  return false;
}

bool Thread::wait_for_exit(unsigned long wait_ms)
{
  assert(false);
  return false;
}

ThreadProcResult
Thread::launch_thread_proc(ThreadProcParam param)
{
    Thread* thread = (Thread*) param;
    ThreadProc thread_proc = thread->m_proc;
    ThreadProcParam thread_param = thread->m_param;
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    return (thread_proc(thread_param));
}
