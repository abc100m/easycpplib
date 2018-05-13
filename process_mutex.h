#ifndef __PROCESS_MUTEX_H__
#define __PROCESS_MUTEX_H__

/*
 * 这个文件主要是想创建一个内核对象, 内核对象是跨进程的
 * come from: http://www.cnblogs.com/jiangwang2013/p/3726097.html
*/

#include "config.h"
#ifdef WIN32                               
    #include <Windows.h>
#else   //linux                             
    #include <semaphore.h>
    #include <unistd.h>
    #include <stdio.h>
    #include <fcntl.h>
    #include <memory.h>
#endif

DUTIL_NAMESPACE_BEGIN

class Process_Mutex
{
public:
    /* 默认创建匿名的互斥 */
    Process_Mutex(const char* name = NULL, bool fail_if_exist = false);
    ~Process_Mutex();

    //判断是否创建成功
    operator bool() const;

    bool lock();
    bool unlock();
    bool try_lock();

    const char* name() const;

private:
    void init_name(const char* name);

private:

#ifdef WIN32                               
    void* mutex_;
#else
    sem_t* mutex_;
#endif
    char mutex_name_[128];
};


#ifdef WIN32

Process_Mutex::Process_Mutex(const char* name, bool fail_if_exist)
{
    init_name(name);
    mutex_ = CreateMutex(NULL, false, mutex_name_);
    if (fail_if_exist && GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(mutex_);
        mutex_ = NULL;
    }
}

Process_Mutex::~Process_Mutex()
{
    if (mutex_) {
        CloseHandle(mutex_);
    }
}

bool Process_Mutex::lock()
{
    if (NULL == mutex_) {
        return false;
    }
    return WAIT_OBJECT_0 == WaitForSingleObject(mutex_, INFINITE);
}

bool Process_Mutex::try_lock()
{
    if (NULL == mutex_) {
        return false;
    }
    return WAIT_OBJECT_0 == WaitForSingleObject(mutex_, 0);
}

bool Process_Mutex::unlock()
{
    if (NULL == mutex_) {
        return false;
    }
    return ReleaseMutex(mutex_);
}

#else

Process_Mutex::Process_Mutex(const char* name, bool fail_if_exist)
{
    init_name(name);

    mode_t m = O_CREAT;
    if (fail_if_exist) {
        m |= O_EXCL;
    }
    mutex_ = sem_open(mutex_name_, m, 0644, 1);
}

Process_Mutex::~Process_Mutex()
{
    if (NULL == mutex_) {
        return;
    }

    int ret = sem_close(mutex_);
    if (0 != ret) {
        printf("sem_close error %d\n", ret);
    }
    sem_unlink(mutex_name_);
}

bool Process_Mutex::lock()
{
    if (NULL == mutex_) {
        return false;
    }
    return 0 == sem_wait(mutex_);
}

bool Process_Mutex::try_lock()
{
    if (NULL == mutex_) {
        return false;
    }
    return 0 == sem_trywait(mutex_);
}

bool Process_Mutex::unlock()
{
    if (NULL == mutex_) {
        return false;
    }
    return 0 == sem_post(mutex_);
}

#endif

void Process_Mutex::init_name(const char* name)
{
    memset(mutex_name_, 0, sizeof(mutex_name_));
    if (name != nullptr) {
        int len = strlen(name);
        int buffer_len = sizeof(mutex_name_) - 1;
        int min = len > buffer_len ? buffer_len : len;
        strncpy(mutex_name_, name, min);
    }
}

const char* Process_Mutex::name() const
{
    return mutex_name_;
}

Process_Mutex::operator bool() const
{
    return NULL != mutex_;
}

DUTIL_NAMESPACE_END

#endif
