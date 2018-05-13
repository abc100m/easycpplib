#include "process_manager.h"
#include <stdlib.h>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#ifdef __STRICT_ANSI__
    #undef __STRICT_ANSI__
    #include <stdio.h>
    #define __STRICT_ANSI__
#else
    #include <stdio.h>
#endif
 

//日志处理
#include "define.h"
using namespace util::log;
using namespace util;

#define debuglog()  MSG_LOG_DEBUG()
#define errorlog()  MSG_LOG_ERROR()


//执行一个命令@cmd，把命令的输出返回到 @out_msg
static int exec_cmd(const char *cmd, std::string& out_msg)
{
    FILE *ptr = popen(cmd, "r");
    if (NULL == ptr) {
        return -1;
    }

    char buf_ps[1024];
    out_msg.clear();
    while (fgets(buf_ps, sizeof(buf_ps), ptr) != NULL) {
        out_msg.append(buf_ps);
    }
    pclose(ptr);

    return 0;
}

//子进程退出后在此收到信号
void child_process_signal(int signal)
{
    while (1) {
        int status;
        pid_t pid = waitpid(-1, &status, WNOHANG);
        if (pid <= 0) {
            break;
        }

        debuglog() << "got a quit signal, pid=" << pid;

        //子进程@pid已经退出, 回调. signal_handler 不能用锁，因此这里开个线程来执行
        typedef std::function<void(pid_t, int)> Fn_Type;
        Fn_Type fn = std::bind(&Process_Manager::do_callback,
            Process_Manager::instance(), std::placeholders::_1, std::placeholders::_2);
        std::thread th(fn, pid, status);
        th.detach();
    }
}
//////////////////////////////////////////////////////////////////////////

Process_Manager::Process_Manager()
    :pre_handler_(NULL)
    ,stopped_(false)
{
}

Process_Manager::~Process_Manager()
{
    stopped_ = true;
    thread_group_.join_all();
}

void Process_Manager::do_callback(pid_t pid, int status)
{
    std::unique_lock<std::mutex> lock(mutex_);
    Hahs_Iterator it = hash_.find(pid);
    if (it != hash_.end()) {
        Child_Exit_Callback fn = it->second;
        if (fn) {
            fn(pid, status);    //回调函数
        }
        hash_.erase(it);
    } 
#if 0
    else {
        debuglog() << "warning::sub process was not in hash_! pid=" << pid;
        hash_[pid] = NULL;     //有可能是子进程先执行又很快结束了, 特殊处理
    }
#endif
}

int Process_Manager::setup_signal()
{
    pre_handler_ = signal(SIGCHLD, child_process_signal);
    if (pre_handler_ == SIG_ERR) {
        errorlog() << "signal error";
        return -1;
    }
    return 0;
}

void Process_Manager::restore_signal()
{
    if (pre_handler_) {
        signal(SIGCHLD, pre_handler_);
    }
}

int Process_Manager::start(const string& work_dir, const string& cmd, 
    /*const string& arg, */Child_Exit_Callback fn, bool kill_old)
{
    if (!kill_old) {
        int n = this->attach(cmd, fn);
        if (0 == n) {
            return 0;
        }
    }
    else {
        //s = "ps -ef|grep " + m_pReptileInfo->strReptileName +"| grep -v grep |grep -v _spider|awk '{print $2}' | xargs pstree -p| awk -F\"[()]\" '{print $2}'|xargs kill -9 2>/dev/null";	
        string s = "ps -efu|grep '" + cmd + "'| grep -v grep |awk '{print $2}'|xargs kill -9 2>/dev/null";
        debuglog() << s;
        ::system(s.c_str());
    }

    pid_t pid = fork();
    if (pid == -1) {
        errorlog() << "fork error";
        return -2;
    }

    //父进程, 返回子进程的id
    if (pid > 0) {
        insert_hash(pid, fn);
        return pid;
    }

    //子进程
    if (pid == (pid_t)0) {
        signal(SIGCHLD, SIG_DFL);  //子进程用系统默认的信号处理方式

        //先暂停2s, 避免父进程还没设置好hash_子进程就退出了
        std::this_thread::sleep_for(std::chrono::seconds(2));

        if (!work_dir.empty()) {
            chdir(work_dir.c_str());
        }

        //int n = execlp(cmd.c_str(), arg.c_str(), NULL);
        int n = execl("/bin/sh", "sh", "-c", cmd.c_str(), (char *)0);
        exit(-1);  //failed
    }

    return 0;
}

int Process_Manager::attach(const string& cmd, Child_Exit_Callback fn)
{
    string s = "ps -efu|grep '" + cmd + "'| grep -v grep |awk '{print $2}'";
    string str_pid;
    int n = exec_cmd(s.c_str(), str_pid);
    if (0 == n && !str_pid.empty()) {
        //进程已存在, 采用轮询方式获取该进程是否退出
        pid_t pid = (pid_t)std::atol(str_pid.c_str());
        insert_hash(pid, fn);

        //typedef boost::function<void (pid_t pid)> Thread_Fn;
        auto f = std::bind(&Process_Manager::poll_wait_in_thread, this, pid);
        thread_group_.create_thread(f);
        return pid;
    }
    return -1;
}

int Process_Manager::stop(pid_t pid, int sig)
{
    return kill(pid, sig);  //0:ok, -1:failed, errno
}

void Process_Manager::stop_all(int sig)
{
    std::unique_lock<std::mutex> lock(mutex_);
    for (Hahs_Iterator it = hash_.begin(); it != hash_.end(); ++it) {
        if (it->second != NULL) {
            kill(it->first, sig);
        }
    }
}

void Process_Manager::poll_wait_in_thread(pid_t pid)
{
    while (!this->stopped_) {
        int n = kill(pid, 0);
        if (-1 == n && errno == ESRCH) {
            debuglog() << "process exited, pid=" << pid;
            this->do_callback(pid, 0);
            break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void Process_Manager::insert_hash(pid_t pid, Child_Exit_Callback fn)
{
    std::unique_lock<std::mutex> lock(mutex_);
    hash_[pid] = fn;

#if 0       //子进程暂停了2S，保证不存在先退出的情况
    Hahs_Iterator it = hash_.find(pid);
    if (it == hash_.end()) {
        hash_.insert(std::make_pair(pid, fn));
    } else {
        debuglog() << "sub process was inserted into hash, pid=" << pid;
        if (it->second == NULL) {  //子进程很快执行并退出的情况
            hash_.erase(it);
            if (fn) {
                debuglog() << "note:: do callback at insert_hash(...), pid=" << pid;
                fn(pid, 0);   //回调, warning::这里无法获取子进程退出时的状态(status), 暂时用0代替
            }
        }
    }
#endif
}


