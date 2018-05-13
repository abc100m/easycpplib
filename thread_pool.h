#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

//from: https://github.com/progschj/ThreadPool
//      http://blog.csdn.net/zdarks/article/details/46994607
//也可参考其它线程池:
//      https://github.com/Tyler-Hardin/thread_pool
//      https://github.com/vit-vit/ctpl

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

class Thread_Pool {
public:
    Thread_Pool(size_t, bool stop_asap = true);
    ~Thread_Pool();

    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args)->std::future<decltype(f(args...))>;

    //返回cpu支持的核心数(双核返回2, 双核4线程返回4)
    inline static unsigned cpu_thread_count() {
        return std::thread::hardware_concurrency();
    }

    void stop() {
        stopped_ = true;
    }

private:
    void schedule_worker();

private:
    // need to keep track of threads so we can join them
    std::vector< std::thread > workers_;
    // the task queue
    std::queue< std::function<void()> > tasks_;
    
    // synchronization
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    volatile bool stopped_;
    volatile bool stop_asap_;   //worker quit ASAP when stopped_ is true, otherwise we should wait worker finish work
};
 
// the constructor just launches some amount of workers
inline Thread_Pool::Thread_Pool(size_t threads, bool stop_asap)
    :stopped_(false)
    ,stop_asap_(stop_asap)
{
    for (size_t i = 0; i<threads; ++i) {
        workers_.emplace_back(&Thread_Pool::schedule_worker, this);
    }
}

void Thread_Pool::schedule_worker()
{
    for (;;) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        condition_.wait(lock, [this]{ return this->stopped_ || !this->tasks_.empty(); });
        if (stopped_ && stop_asap_) {
            return;
        }
        if (!tasks_.empty()) {
            std::function<void()> task = tasks_.front();
            tasks_.pop();
            lock.unlock();
            task();
            continue;
        }
        if (stopped_) {
            return;
        }
    }
}

// add new work item to the pool
template<class F, class... Args>
auto Thread_Pool::enqueue(F&& f, Args&&... args) -> std::future<decltype(f(args...))>
{
    using return_type = decltype(f(args...));

    auto task = std::make_shared< std::packaged_task<return_type()> >(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

    std::future<return_type> res = task->get_future();
    //{
        std::unique_lock<std::mutex> lock(queue_mutex_);

        // don't allow enqueueing after stopping the pool
        if (stopped_) {
            return std::future<return_type>();  //改为不抛异常，但外围要用 valid()检查后再get()
            //throw std::runtime_error("enqueue on stopped ThreadPool");
        }
        tasks_.emplace([task](){ (*task)(); });
    //}
    condition_.notify_one(); //notify前最好是先锁住, see:http://stackoverflow.com/questions/17101922/do-i-have-to-acquire-lock-before-calling-condition-variable-notify-one
    return res;
}

// the destructor joins all threads
inline Thread_Pool::~Thread_Pool()
{
    stopped_ = true;
    condition_.notify_all();
    for (std::thread &worker : workers_) {
        worker.join();
    }
}

#endif
