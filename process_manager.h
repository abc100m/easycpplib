#ifndef _PROCESS_MANAGER_H_
#define _PROCESS_MANAGER_H_

#include <unordered_map>
#include <functional>
#include <mutex>
#include <string>
#include <boost/thread.hpp>

using std::string;

class Process_Manager
{
public:
    //typedef void (*Child_Exit_Fn)(pid_t pid, int status);
    typedef std::function<void(pid_t pid, int status)> Child_Exit_Callback;

    static Process_Manager* instance()
    {
        //dragon:: static 变量在c++11之后线程安全, 在gcc中线程安全, vs2015也支持此规范
        static Process_Manager ins;
        return &ins;
    }

    Process_Manager();
    ~Process_Manager();

    //设置信号
    int setup_signal();

    //恢复信号
    void restore_signal();

public:
    //成功返回pid, 失败返回<0
    int start(const string& work_dir, const string& cmd, /*const string& arg, */Child_Exit_Callback fn, bool kill_old = true);

    //附加到一个命令行, 成功返回进程id，失败返回-1
    int attach(const string& cmd, Child_Exit_Callback fn);

    //kill一个进程, 这是kill -9强制退出.   SIGKILL:9, SIGINT:2, SIGQUIT:3, SIGTERM:15
    int stop(pid_t pid, int sig=9);

    //kill所有进程
    void stop_all(int sig = 9);

protected:
    void do_callback(pid_t pid, int status);
    friend void child_process_signal(int signal);

    //在线程中等待非子进程退出，轮询其状态
    void poll_wait_in_thread(pid_t pid);

    //
    void insert_hash(pid_t pid, Child_Exit_Callback fn);

private:
    typedef std::unordered_map<pid_t, Child_Exit_Callback> Hash;
    typedef Hash::iterator Hahs_Iterator;
    typedef void(*Signal_Handler)(int);

    Hash           hash_;
    std::mutex     mutex_;
    Signal_Handler pre_handler_;
    volatile bool  stopped_;
    boost::thread_group thread_group_;
};

#endif
