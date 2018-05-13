#ifndef _PLOG_INIT_H_
#define _PLOG_INIT_H_

#include "../config.h"
#include "plog_logger.h"
#include "plog_appender.h"

DLOG_NAMESPACE_BEGIN

//////////////////////////////////////////////////////////////////////////
// Empty initializer / one appender

template<int instance>
inline Logger<instance>& init(Severity minSeverity = trace, std::shared_ptr<IAppender> appender = NULL)
{
    Logger<instance>* d = Logger<instance>::instance();
    d->setMinSeverity(minSeverity);
    d->addAppender(appender);
    return *d;
}

inline Logger<PLOG_DEFAULT_INSTANCE>& init(Severity minSeverity = trace, std::shared_ptr<IAppender> appender = NULL)
{
    return init<PLOG_DEFAULT_INSTANCE>(minSeverity, appender);
}

//////////////////////////////////////////////////////////////////////////
// RollingFileAppender with any Formatter

template<class Formatter, int instance>
inline Logger<instance>& init(Severity maxSeverity, const char* fileName, size_t maxFileSize = 0, int maxFiles = 0)
{
    Rolling_File_Appender<Formatter> *appender = new Rolling_File_Appender<Formatter>(fileName, maxFileSize, maxFiles);
    std::shared_ptr<IAppender> f(appender);
    return init<instance>(maxSeverity, f);
}

template<class Formatter>
inline Logger<PLOG_DEFAULT_INSTANCE>& init(Severity maxSeverity, const char* fileName, size_t maxFileSize = 0, int maxFiles = 0)
{
    return init<Formatter, PLOG_DEFAULT_INSTANCE>(maxSeverity, fileName, maxFileSize, maxFiles);
}

template<int instance>
inline Logger<instance>& init(Severity maxSeverity, const char* fileName, size_t maxFileSize = 0, int maxFiles = 0)
{
    return init<TxtFormatter, instance>(maxSeverity, fileName, maxFileSize, maxFiles);
}

inline Logger<PLOG_DEFAULT_INSTANCE>& init(Severity maxSeverity, const char* fileName, size_t maxFileSize = 0, int maxFiles = 0)
{
    return init<PLOG_DEFAULT_INSTANCE>(maxSeverity, fileName, maxFileSize, maxFiles);
}

//////////////////////////////////////////////////////////////////////////
// Data_Size_File_Appender

template<class Formatter, int instance>
inline Logger<instance>& init_ex(Severity maxSeverity, const char* log_path, const char* module, int max_file_size)
{
    Date_Size_File_Appender<Formatter> *appender1 = new Date_Size_File_Appender<Formatter>(log_path, module, max_file_size);
    std::shared_ptr<IAppender> f1(appender1);

    Console_Appender<Formatter> *appender2 = new Console_Appender<Formatter>();
    std::shared_ptr<IAppender> f2(appender2);

    Logger<instance>& d = init<instance>(maxSeverity, f1);
    d.addAppender(f2);

    return d;
}

template<class Formatter>
inline Logger<PLOG_DEFAULT_INSTANCE>& init_ex(Severity maxSeverity, const char* log_path, const char* module, int max_file_size)
{
    return init_ex<Formatter, PLOG_DEFAULT_INSTANCE>(maxSeverity, log_path, module, max_file_size);
}

template<int instance>
inline Logger<instance>& init_ex(Severity maxSeverity, const char* log_path, const char* module, int max_file_size)
{
    return init_ex<TxtFormatter, instance>(maxSeverity, log_path, module, max_file_size);
}

inline Logger<PLOG_DEFAULT_INSTANCE>& init_ex(Severity maxSeverity, const char* log_path, const char* module, int max_file_size)
{
    return init_ex<PLOG_DEFAULT_INSTANCE>(maxSeverity, log_path, module, max_file_size);
}

inline Logger<PLOG_DEFAULT_INSTANCE>& init_ex(Severity maxSeverity=debug)
{
    return init_ex<PLOG_DEFAULT_INSTANCE>(maxSeverity, "./log", "log", 1024*1024*1024);
}

DLOG_NAMESPACE_END

#endif

