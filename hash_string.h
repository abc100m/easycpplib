#ifndef _HASH_STRING_H_
#define _HASH_STRING_H_

#include "config.h"
#include "utils.h"
#include "strutil.h"
#include <string>
#include <unordered_map> 
#include <sstream>
#include <assert.h>
#include <fstream>
#include <vector>

DUTIL_NAMESPACE_BEGIN

using std::string;

template<typename T>
struct identity { typedef T type; };

class Hash_String
{
public:
    Hash_String() {}
    Hash_String(const string& file_name) {
        load_file(file_name);
    }

public:
    inline const string& file_name() const { return file_name_; }

    //获取@key的字符串, 若@key不存在则返回""
    string str(const string& key) const;
    string operator ()(const string& key) const { return str(key); }
    string operator [](const string& key) const { return str(key); }

    //获取值. gcc不支持函数偏特化
    //  see: http://stackoverflow.com/questions/3052579/explicit-specialization-in-non-namespace-scope
    template<typename T>
    T get(const string& key, const T& default_value = T()) const
    {
        return get_private(key, default_value, identity<T>());
    }

    //设置值, int, float, double, long等
    template<typename T>
    void set(const string& key, const T& value)
    {
        std::stringstream ss;
        ss << value;
        hash_[key] = ss.str();
    }

    //设置值, string
    void set(const string& key, const string& value)
    {
        hash_[key] = value;
    }

private:
    //获取值:: int, float, double, long 等
    template<typename T>
    T get_private(const string& key, const T& default_value, identity<T>) const
    {
        Hash_Data_CIterator it = hash_.find(key);
        if (it == hash_.end()) {
            return default_value;
        }

        T t;
        std::stringstream ss(it->second);
        ss >> t;
        return t;
    }

    //获取字符串
    string get_private(const string& key, const string& default_value, identity<string>) const
    {
        Hash_Data_CIterator it = hash_.find(key);
        if (it == hash_.end()) {
            return default_value;
        }
        return it->second;
    }

public:
    /*
    * 文本文件保存的格式为:
    *key=    value
    *        value
    *key2=   value2
    *key3=   value3
    * (注意最后留空行)
    *
    *key前不能留空格，否则该行算作是前一行的延续
    *成功返回0，其它值为失败
    */
    int load_file(const string& file_name);

    //保存到文件, 成功返回0，其它值为失败
    int save_file(const string& file_name = "");

private:
    string file_name_;
    typedef std::unordered_map<string, string>                 Hash_Data;
    typedef std::unordered_map<string, string>::iterator       Hash_Data_Iterator;
    typedef std::unordered_map<string, string>::const_iterator Hash_Data_CIterator;
    Hash_Data hash_;  //id, value
};

std::string Hash_String::str(const string& key) const {
    Hash_Data_CIterator it = hash_.find(key);
    if (it != hash_.end()) {
        return it->second;
    }
    return "";
}

int Hash_String::load_file(const string& file_name)
{
    file_name_ = file_name;
    hash_.clear();

    //从文件读入内容, 这里没有处理编码问题
    string data = NS_DUTIL::load_file_data(file_name_);

    std::vector<string> lst = NS_DSTR::split(data, "\n", true);
    string last_key;

    for (int i = 0, e = lst.size(); i < e; ++i) {
        const string& str = lst.at(i);

        if (str[0] == '#') {
            continue;
        }

        //跳过空行
        if (NS_DSTR::ltrim(str).empty()) {
            continue;
        }

        //有key, value。key 必须顶格写
        if (!std::isspace(str[0])) {
            size_t tab_pos = str.find('=');
            if (tab_pos != string::npos) {
                last_key = NS_DSTR::trim(NS_DSTR::left(str, tab_pos));
                hash_[last_key] = NS_DSTR::ltrim(NS_DSTR::mid(str, tab_pos + 1));
                continue;
            }
        }

        //算作是上一行的延续
        if (!last_key.empty()) {
            hash_[last_key].append("\n").append(str);
        }
    }

    return 0;
}

int Hash_String::save_file(const string& file_name/*=""*/)
{
    string f = file_name.empty() ? file_name_ : file_name;
    if (f.empty()) {
        return -1;
    }
    string txt_data = NS_DUTIL::load_file_data(f);

    std::ofstream ts(f);
    if (!ts) {
        return -2;
    }

    Hash_Data hash_data = hash_;

    //按原文件中的数据顺序回写
    if (!txt_data.empty()) {
        std::vector<string> lst = NS_DSTR::split(txt_data, "\n");
        //去除最后一个空行，以避免多次写入后文件尾的空行会越来越多
        if (!lst.empty() && (*lst.rbegin()).empty()) {
            lst.pop_back();
        }

        for (int i = 0, e = lst.size(); i < e; ++i) {
            const string& str = lst.at(i);
            if (!str.empty() && str[0] != '#' && !NS_DSTR::trim(str).empty())   //注释行和空行不处理
            {
                size_t pos = str.find("=");
                if (pos != string::npos) {
                    string key = NS_DSTR::trim(NS_DSTR::left(str, pos));
                    Hash_Data_Iterator it = hash_data.find(key);
                    if (it != hash_data.end()) {
                        key.append("=").append(it->second);
                        hash_data.erase(it);
                        ts << key << std::endl;     //写入修改后的键值对
                    }
                    else {
                        ts << str << std::endl;    //写入原键值对
                    }
                }
                //不包含键值对的数据将被删除
            }
            else {
                ts << str << std::endl;     //写入注释行和空行
            }
        }
    }

    //原文件中没有的键值都保存到文件尾
    for (Hash_Data_CIterator it = hash_data.begin(); it != hash_data.end(); ++it) {
        ts << it->first << "=" << it->second << std::endl;
    }

    return 0;
}

DUTIL_NAMESPACE_END

#endif
