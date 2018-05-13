#ifndef _PATTERN_H_
#define _PATTERN_H_

#include "config.h"
#include <memory>
#include <mutex>

DUTIL_NAMESPACE_BEGIN

//单件模式
template<typename T>
class Singleton
{
    struct T_Creator {
        T_Creator() { Singleton<T>::s_instance = new T();}

        ~T_Creator() {
            Singleton<T>::destroy();
        }
    };

public:
    static T* instance() 
    {
        //静态变量在多线程中，VC编译不安全(GCC安全)，但c++ 11规定静态变量安全(VC 2015起支持c++11的这个特性)
        static T_Creator t;
        return s_instance;
    }

    //destroy后不能再使用instance，此函数非线程安全
    static void destroy()
    {
        if (s_instance) {
            delete s_instance;
            s_instance = NULL;
        }
    }

protected:
    Singleton() {}
    ~Singleton() {}

private:
    Singleton(const Singleton& );
    Singleton& operator =(const Singleton&);

private:
    static T *s_instance;
};

template<typename T> T* Singleton<T>::s_instance = NULL;
//////////////////////////////////////////////////////////////////////////

//单件模式:: 这是线程安全的版本, 加锁
template<typename T>
class Singleton_Safe
{
public:
    static T* instance() 
    {
        if (NULL == s_instance.get()) {
            std::lock_guard<std::mutex> locker(&s_mutex);
            if (NULL == s_instance.get()) {
                s_instance.reset(new T());
            }
        }
        return s_instance.get();
    }

    static void destroy()
    {
        s_instance.reset(NULL);
    }

protected:
    Singleton_Safe() {}
    ~Singleton_Safe() {}

private:
    Singleton_Safe(const Singleton_Safe& );
    Singleton_Safe& operator =(const Singleton_Safe&);

private:
    static std::unique_ptr<T> s_instance;
    static std::mutex         s_mutex;
};

template<typename T> std::unique_ptr<T>  Singleton_Safe<T>::s_instance;
template<typename T> std::mutex          Singleton_Safe<T>::s_mutex;

DUTIL_NAMESPACE_END

//////////////////////////////////////////////////////////////////////////

#endif
