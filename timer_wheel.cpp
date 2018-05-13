#include "timer_wheel.h"

//for::   struct timeval
#ifdef WIN32
#   include <winsock2.h>
#else
#   include <sys/time.h>
#endif

///////////////////////////以下代码改自linux内核//////////////////////////////////////////
/**
 * Get offset of a member
 */
#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

/**
 * Casts a member of a structure out to the containing structure
 * @param ptr        the pointer to the member.
 * @param type       the type of the container struct this is embedded in.
 * @param member     the name of the member within the struct.
 *
 */
//#define container_of(ptr, type, member) ({                      \
//        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
//        (type *)( (char *)__mptr - offsetof(type,member) );})
#ifndef container_of
#define container_of(ptr, type, member) (type *)((char *)ptr - offsetof(type, member))
#endif 

/**
 * list_entry - get the struct for this entry
 * @ptr:    the &struct list_head pointer.
 * @type:    the type of the struct this is embedded in.
 * @member:    the name of the list_struct within the struct.
 */
#ifndef list_entry
#define list_entry(ptr, type, member) container_of(ptr, type, member)
#endif

/*
 * These are non-NULL pointers that will result in page faults
 * under normal circumstances, used to verify that nobody uses
 * non-initialized list entries.
 */
#ifndef LIST_POISON1
#define LIST_POISON1  ((void *) 0x00100100)
#define LIST_POISON2  ((void *) 0x00200200)
#endif 

#ifndef LIST_HEAD_INIT
#define LIST_HEAD_INIT(name) { &(name), &(name) }
#endif

#ifndef LIST_HEAD
#define LIST_HEAD(name) \
    struct list_head name = LIST_HEAD_INIT(name)
#endif

#ifndef INIT_LIST_HEAD
#define INIT_LIST_HEAD(ptr) do { \
    (ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)
#endif

/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_add(struct list_head *new1,
                  struct list_head *prev,
                  struct list_head *next)
{
    next->prev = new1;
    new1->next = next;
    new1->prev = prev;
    prev->next = new1;
}

/**
 * list_add - add a new entry
 * @new: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void list_add(struct list_head *new1, struct list_head *head)
{
    __list_add(new1, head, head->next);
}

/**
 * list_add_tail - add a new entry
 * @new: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void list_add_tail(struct list_head *new1, struct list_head *head)
{
    __list_add(new1, head->prev, head);
}

/**
* list_replace - replace old entry by new one
* @old : the element to be replaced
* @new : the new element to insert
*
* If @old was empty, it will be overwritten.
*/
static inline void list_replace(struct list_head *old, struct list_head *new1)
{
     new1->next = old->next;
     new1->next->prev = new1;
     new1->prev = old->prev;
     new1->prev->next = new1;
} 

static inline void list_replace_init(struct list_head *old, struct list_head *new1)
{
    list_replace(old, new1);
    INIT_LIST_HEAD(old);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_del(struct list_head * prev, struct list_head * next)
{
    next->prev = prev;
    prev->next = next;
}

/**
 * list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: list_empty on entry does not return true after this, the entry is
 * in an undefined state.
 */
static inline void list_del(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
    entry->next = (list_head*)LIST_POISON1;
    entry->prev = (list_head*)LIST_POISON2;
}

/**
 * list_del_init - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 */
static inline void list_del_init(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
    INIT_LIST_HEAD(entry);
}

/**
 * list_empty - tests whether a list is empty
 * @head: the list to test.
 */
static inline int list_empty(const struct list_head *head)
{
    return head->next == head;
}

///////////////////////////////以上代码抄自linux内核///////////////////////////////////////////

/*
*返回一个timeval结构体表示的毫秒数
*/
inline static unsigned long msc(const timeval* tv)
{
    return tv->tv_sec*1000 + tv->tv_usec/1000;
}

/**
 * timer_pending - is a timer pending?
 * @timer: the timer in question
 *
 * timer_pending will tell whether a given timer is currently pending,
 * or not. Callers must ensure serialization wrt. other operations done
 * to this timer, eg. interrupt contexts, or other CPUs on SMP.
 *
 * return value: 1 if the timer is pending, 0 if not.
 */
static inline int timer_pending(const struct timer_list * timer)
{
    return timer->entry.next != NULL;
}

Timer_Wheel_Impl::Timer_Wheel_Impl()
{
    struct timeval tv;

#ifdef WIN32
    tv = ACE_OS::gettimeofday();   //待移植到linux
#else
    gettimeofday(&tv, NULL);
#endif

    m_base.timer_expire = msc(&tv);

    for(int j = 0; j < TVN_SIZE; j++) {
        INIT_LIST_HEAD(m_base.tv5.vec+ j);
        INIT_LIST_HEAD(m_base.tv4.vec+ j);
        INIT_LIST_HEAD(m_base.tv3.vec+ j);
        INIT_LIST_HEAD(m_base.tv2.vec+ j);
    }

    for(int j = 0; j < TVR_SIZE; j++) {
        INIT_LIST_HEAD(m_base.tv1.vec+ j);
    }
}

void Timer_Wheel_Impl::free_timer_list(struct list_head* head)
{
    struct timer_list* timer;

    while (!list_empty(head)) {
        timer = list_entry(head->next, struct timer_list, entry);
        detach_timer(timer, 1);
        delete timer;
    }
}

Timer_Wheel_Impl::~Timer_Wheel_Impl()
{
    //析构函数中释放所有定时器
    for (int j = 0; j < TVN_SIZE; j++) {
        free_timer_list(m_base.tv5.vec+ j);
        free_timer_list(m_base.tv4.vec+ j);
        free_timer_list(m_base.tv3.vec+ j);
        free_timer_list(m_base.tv2.vec+ j);
    }

    for (int j = 0; j < TVR_SIZE; j++) {
        free_timer_list(m_base.tv1.vec+ j);
    }
}

int Timer_Wheel_Impl::internal_add_timer(struct timer_list *timer)
{
    unsigned long expires = timer->expires;
    unsigned long idx = expires - m_base.timer_expire;
    struct list_head *vec;

    if (idx < TVR_SIZE) {
        int i = expires & TVR_MASK;
        vec = m_base.tv1.vec + i;
    } else if (idx < 1 << (TVR_BITS + TVN_BITS)) {
        int i = (expires >> TVR_BITS) & TVN_MASK;
        vec = m_base.tv2.vec + i;
    } else if (idx < 1 << (TVR_BITS + 2 * TVN_BITS)) {
        int i = (expires >> (TVR_BITS + TVN_BITS)) & TVN_MASK;
        vec = m_base.tv3.vec + i;
    } else if (idx < 1 << (TVR_BITS + 3 * TVN_BITS)) {
        int i = (expires >> (TVR_BITS + 2 * TVN_BITS)) & TVN_MASK;
        vec = m_base.tv4.vec + i;
    } else if ((signed long) idx < 0) {
        /*
         * Can happen if you add a timer with expires == jiffies,
         * or you set a timer to go off in the past
         */
        vec = m_base.tv1.vec + (m_base.timer_expire & TVR_MASK);
    } else {
        int i;
        /* If the timeout is larger than 0xffffffff on 64-bit
         * architectures then we use the maximum timeout:
         */
        if (idx > 0xffffffffUL) {
            idx = 0xffffffffUL;
            expires = idx + m_base.timer_expire;
        }
        i = (expires >> (TVR_BITS + 3 * TVN_BITS)) & TVN_MASK;
        vec = m_base.tv5.vec + i;
    }

    /*
     * Timers are FIFO:
     */
    list_add_tail(&timer->entry, vec);

    return 0;
}

struct timer_list* Timer_Wheel_Impl::add_timer(Callback_Function fn, 
                                               const timeval* expire_tv, 
                                               unsigned long interval
                                            )
{
    timer_list* timer = free_list_.alloc();

    if (timer) {
        timer->fn       = fn;
        timer->expires  = msc(expire_tv);  //ms
        timer->interval = interval;
        internal_add_timer(timer);
    }

    return timer;
}

//类似于list_for_each_entry, 这样改写只是为支持没有typeof的编译器
#define travel_timer_list_safe(pos, n, head, member)            \
    for (pos = list_entry((head)->next, struct timer_list, member),    \
        n = list_entry(pos->member.next, struct timer_list, member);    \
         &pos->member != (head);                     \
         pos = n, n = list_entry(n->member.next, struct timer_list, member))


int Timer_Wheel_Impl::cascade(struct tvec *tv, int index)
{
    /* cascade all the timers from tv up one level */
    struct timer_list *timer, *tmp;
    struct list_head tv_list;

    list_replace_init(tv->vec + index, &tv_list);

    /*
     * We are removing _all_ timers from the list, so we
     * don't have to detach them individually.
     */
//    list_for_each_entry_safe(timer, tmp, &tv_list, entry) {
    travel_timer_list_safe(timer, tmp, &tv_list, entry)
    {
        internal_add_timer(timer);
    }

    return index;
}

#define time_after(a,b) ((long)(b) - (long)(a) < 0) 
#define time_before(a,b) time_after(b,a) 

#define time_after_eq(a,b) ((long)(a) - (long)(b) >= 0) 
#define time_before_eq(a,b) time_after_eq(b,a)

#define INDEX(N) ((m_base.timer_expire >> (TVR_BITS + (N) * TVN_BITS)) & TVN_MASK)

/**
 * __run_timers - run all expired timers (if any) on this CPU.
 * @base: the timer vector to be processed.
 *
 * This function cascades all vectors and executes all expired timer
 * vectors.
 */
int Timer_Wheel_Impl::expire( const timeval* tv )
{
    struct timer_list *timer;
    unsigned long expire = msc(tv);

    while (time_after_eq(expire, m_base.timer_expire)) {
        struct list_head work_list;
        struct list_head *head = &work_list;
        int index = m_base.timer_expire & TVR_MASK;

        /*
         * Cascade timers:
         */
        if (!index &&
             (!cascade(&m_base.tv2, INDEX(0))) &&
                (!cascade(&m_base.tv3, INDEX(1))) &&
                    !cascade(&m_base.tv4, INDEX(2))
           )
        {
            cascade(&m_base.tv5, INDEX(3));
        }

        list_replace_init(m_base.tv1.vec + index, &work_list);

        ++m_base.timer_expire;

        //遍历执行所有的超时定时器
        while (!list_empty(head)) {
            timer = list_entry(head->next, struct timer_list, entry);
            Callback_Function fn = timer->fn;

            detach_timer(timer, 1);
            if (timer->interval > 0) {  
                //周期性定时器
                //考虑间隔为1s的定时器，10s后调用expire，现在则回调10次。
                //考虑: 此处可以将timer->expires加到大于tv的值， 则回调1次
                timer->expires += timer->interval; 
                internal_add_timer(timer);
            } else {
                //一次性定时器, 回收。
                //注意：此处已将定时器删除，外层应避免再次调用cancel_timer取消此定时器，否则会出错
                free_list_.free(timer); 
            }

            //printf("%u trig a timer\n", m_base.timer_expire - 1/*msc(ACE_OS::gettimeofday())*/);
            fn();
        }
    }

    return 0;
}

inline void Timer_Wheel_Impl::detach_timer(struct timer_list *timer, int clear_pending)
{
    struct list_head *entry = &timer->entry;

    __list_del(entry->prev, entry->next);

    if (clear_pending) {
        entry->next = NULL;
    }

    entry->prev = (list_head*)LIST_POISON2;
}

int Timer_Wheel_Impl::mod_timer(struct timer_list *timer, unsigned long expires)  
{
    /*
     * This is a common optimization triggered by the 
     * networking code - if the timer is re-modified 
     * to be the same thing then just return: 
     */
    if (timer_pending(timer) && timer->expires == expires) {
        return 1;
    }

    if (timer_pending(timer)) {
        detach_timer(timer, 0);
    }

    timer->expires = expires;
    internal_add_timer(timer);

    return 0;
}

int Timer_Wheel_Impl::remove_timer( struct timer_list* timer )
{
/*考虑这种情况：
  add_timer返回句柄，expire()后timeout了(隐式调用了remove_timer), 此时之前的句柄已无效。
  如果再调用remove_timer, 这就乱了!
  外围应该在timeout回调中处理这种情况
*/
    if (timer_pending(timer)) {
        detach_timer(timer, 1);
    }

    free_list_.free(timer);
    return 0;
}
///////////////////////////////////////////////////////////////////////////////////////

//class Free_timer_list
Free_Timer_List::~Free_Timer_List()
{
    for (std::list<struct timer_list*>::iterator it = free_list_.begin();
         it != free_list_.end();
         ++it
        )
    {
        delete *it;
    }
}

struct timer_list* Free_Timer_List::alloc()
{
    if (!free_list_.empty()) {
        timer_list* p = free_list_.front();
        free_list_.pop_front();
        return p;
    }

    return new timer_list();
}

void Free_Timer_List::free( struct timer_list* timer)
{
    //free_list_中最多保留2048个
    if (free_list_.size() > 2048) {
        delete timer;
    } else {
        timer->fn = nullptr;
        free_list_.push_back(timer);
    }
}

