#ifndef THREAD_HPP
#define THREAD_HPP


#include <pthread.h>
#include <sys/time.h>


typedef void* ThreadProcParam;
typedef void* ThreadProcResult;
typedef ThreadProcResult(*ThreadProc)(ThreadProcParam);

class Thread
{
public:
    Thread(ThreadProc proc, ThreadProcParam param, bool is_detached);
    ~Thread();

public:
    bool start();
    bool stop();
    bool wait_for_exit(unsigned long wait_ms);

protected:
    static ThreadProcResult launch_thread_proc(ThreadProcParam param);

protected:
    pthread_t m_thread;
    bool m_is_detached;
    ThreadProc m_proc;
    ThreadProcParam m_param;
};


#endif
