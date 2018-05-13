#ifndef _PLOG_APPENDER_H_
#define _PLOG_APPENDER_H_

#include "../config.h"
#include "plog_logger.h"
#include <iostream>
#include <string>
#include <sstream>
#include <mutex>
#include <fstream>
#include <cstring>


DLOG_NAMESPACE_BEGIN


template<class Formatter>
class Console_Appender : public IAppender
{
public:
    Console_Appender() {}

    virtual void write(const Record& record)
    {
        std::cout << Formatter::format(record) << std::flush;
    }
};

//plog自带的日志函数，未经测试
template<class Formatter>
class Rolling_File_Appender : public IAppender
{
public:
    Rolling_File_Appender(const char* fileName, size_t maxFileSize = 0, int maxFiles = 0)
        : m_fileSize()
        , m_maxFileSize((std::max)(maxFileSize, static_cast<size_t>(1000))) // set a lower limit for the maxFileSize
        , m_lastFileNumber((std::max)(maxFiles - 1, 0))
        , m_firstWrite(true)
    {
        const char* dot = std::strrchr(fileName, '.');
        if (dot) {
            m_fileNameNoExt.assign(fileName, dot);
            m_fileExt.assign(dot + 1);
        } else {
            m_fileNameNoExt.assign(fileName);
            m_fileExt.clear();
        }
    }

    virtual void write(const Record& record)
    {
        std::unique_lock<std::mutex> lock(m_mutex);

        if (m_firstWrite) {
            openLogFile();
            m_firstWrite = false;
        }
        else if (m_lastFileNumber > 0 && 
                 m_fileSize > m_maxFileSize && 
                 static_cast<size_t>(-1) != m_fileSize
                ) {
            rollLogFiles();
        }

        const std::string& log = Formatter::format(record);
        m_file.write(log.c_str(), log.size());
        m_fileSize += log.size();
    }

private:
    void rollLogFiles()
    {
        m_file.close();

        string lastFileName = buildFileName(m_lastFileNumber);
        ::unlink(lastFileName.c_str());

        for (int fileNumber = m_lastFileNumber - 1; fileNumber >= 0; --fileNumber) {
            string currentFileName = buildFileName(fileNumber);
            string nextFileName = buildFileName(fileNumber + 1);
            ::rename(currentFileName.c_str(), nextFileName.c_str());
        }

        openLogFile();
    }

    void openLogFile()
    {
        string fileName = buildFileName();
        m_file.open(fileName.c_str(), std::ios::app|std::ios::binary);
        if (!m_file) {
            std::cerr << "open file failed: " << fileName;
        }
        m_fileSize = m_file.tellp();

        if (0 == m_fileSize) {
            const std::string& log = Formatter::header();
            m_file.write(log.c_str(), log.size());
            m_fileSize += log.size();
        }
    }

    string buildFileName(int fileNumber = 0)
    {
        std::stringstream ss;
        ss << m_fileNameNoExt;
        
        if (fileNumber > 0) {
            ss << '.' << fileNumber;
        }
        if (!m_fileExt.empty()) {
            ss << '.' << m_fileExt;
        }

        return ss.str();
    }

private:
    std::mutex      m_mutex;
    std::ofstream   m_file;
    size_t          m_fileSize;
    const size_t    m_maxFileSize;
    const int       m_lastFileNumber;
    string          m_fileExt;
    string          m_fileNameNoExt;
    bool            m_firstWrite;
};

//按天按大小输出日志文件
template<class Formatter>
class Date_Size_File_Appender : public IAppender
{
public:
    Date_Size_File_Appender()
        :Date_Size_File_Appender("./log", "log", 1024 * 1024 * 1024)
    {
    }

    Date_Size_File_Appender(const std::string& log_path, const std::string& module, int max_size)
        :log_path_(log_path)
        ,module_(module)
        ,max_size_(max_size)
    {
        current_size_ = 0;
        backup_count_ = 0;
        today_        = time(NULL);
    }

public:

    virtual void write(const Record& record)
    {
        time_t now = time(NULL);
        int d1 = now / 86400;
        int d2 = today_ / 86400;
        if (d1 != d2 || current_size_ >= max_size_) {
            file_.close();
            if (d1 != d2) {
                backup_count_ = 0;
            } else {
                backup_count_ += 1;
            }
            
            current_size_  = 0;
            today_         = now;
        }

        if (!file_.is_open()) {
            std::string filename = calc_file_name();
            file_.open(filename, std::ios::app|std::ios::binary);
        }

        const std::string& data = Formatter::format(record);
        file_ << data << std::flush;
        current_size_ += data.length();
    }

private:
    std::string calc_file_name()
    {
        if (!log_util::dir_exists(log_path_)) {
#ifdef __MINGW32__
            ::mkdir(log_path_.c_str());
#else
            ::mkdir(log_path_.c_str(), 0755);
#endif
        }

        char today_buf[64];
        time_t now = time(NULL);
        struct tm t;
        log_util::localtime_s(&t, &now);
        strftime(today_buf, sizeof(today_buf), "%Y%m%d", &t);

        std::ostringstream  ss;
        ss << log_path_ << "/" << module_ << today_buf;
        if (backup_count_ > 0) {
            ss << "_" << backup_count_;
        }
        ss << ".log";

        std::string file_name = ss.str();
        if (log_util::file_exists(file_name) && log_util::file_size(file_name) >= max_size_) {
            backup_count_ += 1;
            return calc_file_name();
        }
        return file_name;
    }

public:
    void set_current_size(int s) {
        current_size_ = s;
    }

    void set_max_size(int s) {
        max_size_ = s;
    }

    void set_log_path(const std::string& s) {
        log_path_ = s;
    }

    void set_module(const std::string& s) {
        module_ = s;
    }

    int current_size() const {
        return current_size_;
    }

    int max_size() const {
        return max_size_;
    }

    std::string log_path() const {
        return log_path_;
    }

    std::string module() const {
        return module_;
    }

private:
    int         current_size_;  //当前文件大小
    int         max_size_;      //文件的最大大小，单位:字节
    int         backup_count_;  //备份数
    std::string log_path_;      //日志目录
    std::string module_;        //模块名
    time_t      today_;         //当前日期
    std::ofstream file_;
};


DLOG_NAMESPACE_END

#endif
