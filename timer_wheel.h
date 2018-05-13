#ifndef _TIMER_WHEEL_H_
#define _TIMER_WHEEL_H_

#include <list>
#include <functional>

struct timeval;

//-------------------------------以下宏及结构体请参考linux内核-------------------------------
#define TVN_BITS 6 //(CONFIG_BASE_SMALL ? 4 : 6)
#define TVR_BITS 8 //(CONFIG_BASE_SMALL ? 6 : 8)
#define TVN_SIZE (1 << TVN_BITS)
#define TVR_SIZE (1 << TVR_BITS)
#define TVN_MASK (TVN_SIZE - 1)
#define TVR_MASK (TVR_SIZE - 1)

/**
 * Simple doubly linked list implementation.
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole lists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */
struct list_head {
	struct list_head *next, *prev;
};

struct tvec {
    struct list_head vec[TVN_SIZE];
};

struct tvec_root {
    struct list_head vec[TVR_SIZE];
};

typedef std::function<void()> Callback_Function;

struct timer_list {
    struct list_head entry;
    unsigned long expires;
    unsigned long interval;     //重复周期，0-一次性定时器，其它值为周期性定时器
    Callback_Function fn;       //回调函数. 采用c++ 11的function导致很多编译警告, raw function ptr就没有警告
};

struct tvec_base {
    unsigned long timer_expire;
    struct tvec_root tv1;
    struct tvec tv2;
    struct tvec tv3;
    struct tvec tv4;
    struct tvec tv5;
};
//-------------------------------以上代码参考linux内核-------------------------------

class Free_Timer_List
{
public:
    ~Free_Timer_List();

public:
    struct timer_list* alloc();
    void free(struct timer_list*);

private:
    std::list<struct timer_list*> free_list_;
};

class Timer_Wheel_Impl
{
public:
    Timer_Wheel_Impl();
    ~Timer_Wheel_Impl();

public:
    struct timer_list* add_timer(
                                Callback_Function fn,
                                const timeval* expire_tv,
                                unsigned long interval
                                );
    int remove_timer(struct timer_list* timer);
    int expire(const timeval* tv);

protected:
    int mod_timer(struct timer_list *timer, unsigned long expires);
    int internal_add_timer(struct timer_list *timer);
    void detach_timer(struct timer_list *timer, int clear_pending);

    int cascade(struct tvec *tv, int index);
    void free_timer_list(struct list_head* head);

private:
    struct tvec_base m_base;
    Free_Timer_List free_list_;   //用于缓存timer_list;
};
//////////////////////////////////////////////////////////////////////////

//////////////////////  下面是对外提供的接口, 原来是分2个文件现在合在一起了 ///////////////////////////////////
typedef void* Timer_handle;

class Timer_wheel: private Timer_Wheel_Impl
{
public:
    //新增一个定时器, 返回NULL为失败, 其它值为成功。欲取消此定时器，调用cancel_timer
    template<typename F, typename ...Args>
    Timer_handle schedule(
                         const timeval* expire_tv,     //超时时间
                         unsigned long interval,       //0:一次性定时器; >0 :周期性定时器. 单位：毫秒
                         F&& f,
                         Args&& ...args
                         )
    {
        //using return_type = decltype(f(args...));
        auto task = std::bind(f, args...);
        return (Timer_handle)Timer_Wheel_Impl::add_timer(task, expire_tv, interval);
    }

    //取消一个定时器. 0-成功，其它值-失败
    int cancel_timer(Timer_handle timer) {
        if (timer) {
            return Timer_Wheel_Impl::remove_timer((timer_list*)timer);
        }
        return 0;
    }

    //循环调用此函数，触发所有过期的定时器, tv值可取当前系统时间
    int expire(const timeval* tv) {
        return Timer_Wheel_Impl::expire(tv);
    }

};


#endif

