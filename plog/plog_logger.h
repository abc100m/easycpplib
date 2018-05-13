#ifndef _PLOG_LOGGER_H_
#define _PLOG_LOGGER_H_

#ifndef PLOG_DEFAULT_INSTANCE
    #define PLOG_DEFAULT_INSTANCE 0
#endif

#include "../config.h"
#include "../pattern.h"
#include "../thread_pool.h"
#include <sstream>
#include <string>
#include <time.h>
#include <stdio.h>
#include <assert.h>
#include <sys/stat.h>
#include <memory>
#include <utility>

#ifdef WIN32
#   include <Windows.h>
#   include <time.h>
#   include <sys/timeb.h>
#   include <io.h>
#   include <share.h>
#else
#   include <unistd.h>

#if !defined( __MINGW32__ )     //mingw-64项目的mingw编译，不会定义WIN32宏
    #include <sys/syscall.h>
#else
    #include <windows.h>    //这个头文件包含后会定义WIN32
    #undef  WIN32
#endif

#   include <sys/time.h>
#   include <pthread.h>
#endif

//是否开启一个独立的线程来写日志?
#define DLOG_SEPARATE_THREAD 1

DLOG_NAMESPACE_BEGIN

//////////////////      log_util        /////////////////////////////////////////
//帮助函数, 放在log_util
namespace log_util {

#ifdef WIN32  //mingw-64 编译, 没有定义WIN32, 有 gettimeofday 函数
    typedef timeb Time;

    inline void ftime(Time* t)
    {
        ::ftime(t);
    }

#else

    struct Time
    {
        time_t         time;
        unsigned short millitm;
    };

    inline void ftime(Time* t)
    {
        struct timeval tv;
        ::gettimeofday(&tv, NULL);

        t->time = tv.tv_sec;
        t->millitm = static_cast<unsigned short>(tv.tv_usec / 1000);
    }
#endif

    inline unsigned int get_thread_id()
    {
#if  defined WIN32 ||  defined __MINGW32__
        return ::GetCurrentThreadId();
#elif defined(__unix__)
#ifndef __MINGW32__
        return ::syscall(__NR_gettid);
#else
        return -1;
#endif
#elif defined(__APPLE__)
        return static_cast<unsigned int>(::syscall(SYS_thread_selfid));
#endif
    }

    inline void localtime_s(struct tm* t, const time_t* time)
    {
#if defined(WIN32) && defined(__BORLANDC__)
        ::localtime_s(time, t);
#elif defined(WIN32)
        ::localtime_s(t, time);
#else
        ::localtime_r(time, t);
#endif
    }

    inline std::stringstream& operator<<(std::stringstream& stream, const char* data)
    {
        std::operator<< (stream, (data ? data : "(null)"));
        return stream;
    }

    inline std::stringstream& operator<<(std::stringstream& stream, const std::string& data)
    {
        std::operator<<(stream, !data.empty() ? data.c_str() : "(null)");
        return stream;
    }

    inline bool dir_exists(const std::string &fname)
    {
        struct stat sbuf;
        return stat(fname.c_str(), &sbuf) == 0 && S_ISDIR(sbuf.st_mode);
    }

    inline bool file_exists(const std::string &fname)
    {
        struct stat sbuf;
        return stat(fname.c_str(), &sbuf) == 0 && S_ISREG(sbuf.st_mode);
    }

    inline long file_size(const std::string& filename)
    {
        struct stat stat_buf;
        int rc = stat(filename.c_str(), &stat_buf);
        return rc == 0 ? stat_buf.st_size : -1;
    }
}

/////////////////////// Severity   ///////////////////////////////////
enum Severity
{
    trace = 0,
    debug,
    info,
    warning,
    error,
    fatal,
    none
};

inline const char* severityToString(Severity severity)
{
    switch (severity) {
    case fatal:
        return "FATAL";
    case error:
        return "ERROR";
    case warning:
        return "WARN";
    case info:
        return "INFO";
    case debug:
        return "DEBUG";
    case trace:
        return "TRACE";
    default:
        return "NONE";
    }
}

inline Severity severityFromString(const char* str)
{
    for (Severity severity = trace; severity <= fatal; severity = static_cast<Severity>(severity + 1)) {
        if (severityToString(severity)[0] == str[0]) {
            return severity;
        }
    }
    return none;
}    

////////////////////////    Record      ///////////////////////////////////////
class Record
{
    struct Record_Data {
        log_util::Time      m_time;
        const Severity      m_severity;
        const unsigned int  m_tid;
        const void* const   m_object;
        const char* const   m_file_name;
        const size_t        m_line;
        const char* const   m_func;
        std::stringstream   m_message;

        inline Record_Data(Severity severity, const char* file_name, const char* func, size_t line, const void* object)
            : m_severity(severity), m_tid(log_util::get_thread_id()), m_object(object)
            , m_file_name(file_name), m_line(line), m_func(func)
        {
            log_util::ftime(&m_time);
        }
    };

    inline Record(Record_Data *d)
        : data_(d)
    {}

    template<int>
    friend class Logger;

public:
    inline Record(Severity severity, const char* file_name, const char* func, size_t line, const void* object)
        : data_(new Record_Data(severity, file_name, func, line, object))
    {}

    ~Record() {
        if (data_) {
            delete data_;
        }
    }

    inline Record(Record && r) noexcept
           :data_(r.data_)
    {
        r.data_ = nullptr;
    }

    Record& operator<<(char data)
    {
        char str[] = { data, 0 };
        *this << str;
        return *this;
    }

    template<typename T>
    Record& operator<<(const T& data)
    {
        using namespace log_util;
        data_->m_message << data;
        return *this;
    }

    const log_util::Time& getTime() const
    {
        return data_->m_time;
    }

    Severity getSeverity() const
    {
        return data_->m_severity;
    }

    unsigned int getTid() const
    {
        return data_->m_tid;
    }

    const void* getObject() const
    {
        return data_->m_object;
    }

    const char* getFileName() const
    {
        return data_->m_file_name;
    }

    size_t getLine() const
    {
        return data_->m_line;
    }

    const std::string getMessage() const
    {
        return data_->m_message.str();
    }

    std::string getFunc() const
    {
        const char* func = data_->m_func;
#if (defined(WIN32) && !defined(__MINGW32__)) || defined(__OBJC__)
        return std::string(func);
#else
        //函数格式：int Cls::foo(int, double), 这里仅取出Cls::foo
        const char* funcBegin = func;
        const char* funcEnd   = ::strchr(funcBegin, '(');

        if (!funcEnd) {
            return std::string(func);
        }

        for (const char* i = funcEnd - 1; i >= funcBegin; --i) { // search backwards for the first space char
            if (*i == ' ') {
                funcBegin = i + 1;
                break;
            }
        }

        return std::string(funcBegin, funcEnd);
#endif
    }

private:
    Record_Data *data_;
};  

//////////////////       Formatter      ////////////////////////////////////
class TxtFormatter
{
public:
    static std::string header()
    {
        return std::string();
    }

    static std::string format(const Record& record)
    {
        tm t;
        log_util::localtime_s(&t, &record.getTime().time);

        char buf[64];
        int n = strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &t);
        sprintf(buf + n, ".%.3hu", record.getTime().millitm);
        
        std::stringstream ss;
        ss << buf << " " << severityToString(record.getSeverity()) << " ";
        //ss << std::setfill(PLOG_NSTR(' ')) << std::setw(5) << std::left <<  << " ";

        ss << "[" << record.getTid() << "] ";
        ss << "[" << record.getFunc().c_str() << "@" << record.getFileName() << "/" << record.getLine() << "] ";
        ss << record.getMessage().c_str() << "\n";

        return ss.str();
    }
};

/////////////////   IAppender //////////////////////////////////////////////
class IAppender
{
public:
    virtual ~IAppender() {}
    virtual void write(const Record& record) = 0;
};


////////////////////    Logger   /////////////////////////////////////////////
template<int instance>
class Logger : public NS_DUTIL::Singleton<Logger<instance> >
{
public:
    Logger(Severity minSeverity = debug) 
        : m_minSeverity(minSeverity)
#ifdef DLOG_SEPARATE_THREAD
        , pool_(1, false)
#endif
    {
    }

    Logger& addAppender(std::shared_ptr<IAppender> appender)
    {
        m_appenders.push_back(appender);
        return *this;
    }

    Severity getMinSeverity() const
    {
        return m_minSeverity;
    }

    void setMinSeverity(Severity severity)
    {
        m_minSeverity = severity;
    }

    bool checkSeverity(Severity severity) const
    {
        return severity >= m_minSeverity;
    }

#ifndef DLOG_SEPARATE_THREAD
    void operator+=(const Record& record)
    {
        //note::也许这里加锁更合适, 但plog的作者把锁放到Appender中了. 
        //      我偏向于使用单独的线程,所以没有修改这里
        log_to_appender(record);
    }
#else
    void operator+=(Record& record)
    {
        //auto f = [this](Record::Record_Data* p){
        //    this->log_to_appender(Record(p));
        //};
        //auto ptr = record.data_;
        //record.data_ = nullptr;
        //pool_.enqueue(f, ptr);

        auto f = [this](const Record& r){
            this->log_to_appender(r);
        };
        pool_.enqueue(f, std::move(record));
    }
#endif

    inline void log_to_appender(const Record& record)
    {
        //std::vector<std::shared_ptr<IAppender>>::iterator
        for (auto it = m_appenders.begin(); it != m_appenders.end(); ++it) {
            (*it)->write(record);
        }
    }

private:
    Severity m_minSeverity;
    std::vector<std::shared_ptr<IAppender> > m_appenders;
#ifdef DLOG_SEPARATE_THREAD
    Thread_Pool pool_;
#endif
};

template<int instance>
inline Logger<instance>* get()
{
    return Logger<instance>::instance();
}

inline Logger<PLOG_DEFAULT_INSTANCE>* get()
{
    return Logger<PLOG_DEFAULT_INSTANCE>::instance();
}


DLOG_NAMESPACE_END

#endif

