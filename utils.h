#ifndef _UTILS_H_
#define _UTILS_H_

#include "config.h"
#include <sys/stat.h>
#include <string>

#ifdef WIN32
    #include <io.h>         //use close
    #include <direct.h>   //getcwd
#else
    #include <unistd.h>     //use close
#endif

#undef  __STRICT_ANSI__
#include <stdio.h>        //FILENAME_MAX
#include <fstream> 
#include <sstream>

DUTIL_NAMESPACE_BEGIN

using std::string;

/// Allow stat to work directly on C++ strings.
inline int stat(const string &filename, struct stat *buf) {
    return stat(filename.c_str(), buf);
}

/* get file size
*/
long get_file_size(const char *path)
{
    struct stat statbuff;
    if (stat(path, &statbuff) < 0) {
        return -1;
    }
    return statbuff.st_size;
}

inline long get_file_size(const string& path)
{
    return get_file_size(path.c_str());
}

/** Return true if the file fname exists.
 */
bool file_exists(const string &fname) {
    struct stat sbuf;
    // exists && is a regular file
    return stat(fname, &sbuf) == 0 && S_ISREG(sbuf.st_mode);
}

/** Return true if the directory dirname exists.
 */
bool dir_exists(const string &dirname) {
    struct stat sbuf;
    // exists && is a directory
    return stat(dirname, &sbuf) == 0 && S_ISDIR(sbuf.st_mode);
}

//当前路径, 没有最后的 /
string current_dir() {
    char buff[FILENAME_MAX];
    ::getcwd(buff, sizeof(buff));
    return string(buff);
}

/*读取文件读入数据. 若文件不存在返回为空(文件没有数据也是空)
* 请使用 @file_exists 判断文件是否存在
*/
string load_file_data(const char* file_name) {
    std::ifstream f(file_name);
    std::stringstream ss;
    ss << f.rdbuf();//read the file
    return ss.str();
}

inline string load_file_data(const string& file_name) {
    return load_file_data(file_name.c_str());
}


//将数据写入到文件
int save_data_to_file(const char* filename, const char* data, size_t size) {
    FILE* fp = fopen(filename, "wb+");
    if (fp) {
        fwrite(data, size, 1, fp);
        fclose(fp);
        return 0;
    }
    return -1;
}

inline int save_data_to_file(const char* filename, const string& data) {
    return save_data_to_file(filename, data.c_str(), data.length());
}

inline int save_data_to_file(const string& filename, const string& data) {
    return save_data_to_file(filename.c_str(), data.c_str(), data.size());
}

/*
    执行系统命令@cmd, 将输出返回到@out_msg.
    成功返回0，其它值为失败
*/
int exec_cmd(const char *cmd, std::string& out_msg) {
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

//判断是否为utf8
bool isutf8(const void* pBuffer, long size)
{
    bool ret = true;
    unsigned char* start = (unsigned char*)pBuffer;
    unsigned char* end = (unsigned char*)pBuffer + size;
    while (start < end) {
        if (*start < 0x80) { // (10000000): 值小于0x80的为ASCII字符
            start++;
        }
        else if (*start < (0xC0)) { // (11000000): 值介于0x80与0xC0之间的为无效UTF-8字符
            ret = false;
            break;
        }
        else if (*start < (0xE0)) { // (11100000): 此范围内为2字节UTF-8字符
            if (start >= end - 1) break;
            if ((start[1] & (0xC0)) != 0x80) {
                ret = false;
                break;
            }
            start += 2;
        }
        else if (*start < (0xF0)) { // (11110000): 此范围内为3字节UTF-8字符
            if (start >= end - 2) break;
            if ((start[1] & (0xC0)) != 0x80 || (start[2] & (0xC0)) != 0x80) {
                ret = false;
                break;
            }
            start += 3;
        }
        else {
            ret = false;
            break;
        }
    }
    return ret;
}


/** A tiny class used to close a filehandle safely in the presence
 *  of exceptions.
 */
class fdcloser {
    public:
	fdcloser(int fd_) : fd(fd_) {}
	~fdcloser() {
	    if (fd >= 0) {
		    close(fd);
	    }
	}
    private:
	    int fd;
};

DUTIL_NAMESPACE_END

#endif 

