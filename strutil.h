#ifndef _STR_UTIL_H_
#define _STR_UTIL_H_

#include "config.h"
#include <string>
#include <memory.h>
#include <vector>
#include <sstream>
#include <iomanip>
#include <algorithm>  //std::transform
#include <cstdio>    // For snprintf() or sprintf().
#include <cstdlib>   // For abort().
#include <ctype.h>   //这里特意要用C库的::tolower, ::toupper
#include <stdarg.h>  //va_arg
#include <string.h>
#include <cassert>


//code from xapian  http://xapian.org/

/** Helper macro for STRINGIZE - the nested call is required because of how
 *  # works in macros.
 */
#define STRINGIZE_(X) #X

/// The STRINGIZE macro converts its parameter into a string constant.
#define STRINGIZE(X) STRINGIZE_(X)

/** Returns the length of a string constant.
 *
 *  We rely on concatenation of string literals to produce an error if this
 *  macro is applied to something other than a string literal.
 */
#define CONST_STRLEN(S) (sizeof(S"") - 1)

DSTR_NAMESPACE_BEGIN

using std::string;
using std::vector;

//std::to_string

inline bool startswith(const string & s, char pfx) {
    return !s.empty() && s[0] == pfx;
}

inline bool startswith(const string & s, const char * pfx, size_t len) {
    return s.size() >= len && (::memcmp(s.data(), pfx, len) == 0);
}

inline bool startswith(const string & s, const char * pfx) {
    return startswith(s, pfx, ::strlen(pfx));
}

inline bool startswith(const string & s, const string & pfx) {
    return startswith(s, pfx.data(), pfx.size());
}

inline bool endswith(const string & s, char sfx) {
    return !s.empty() && s[s.size() - 1] == sfx;
}

inline bool endswith(const string & s, const char * sfx, size_t len) {
    return s.size() >= len && (::memcmp(s.data() + s.size() - len, sfx, len) == 0);
}

inline bool endswith(const string & s, const char * sfx) {
    return endswith(s, sfx, ::strlen(sfx));
}

inline bool endswith(const string & s, const string & sfx) {
    return endswith(s, sfx.data(), sfx.size());
}

//使用C++ 11的标准库 std::to_string。原来从 xapian 摘取的 tostr 就去除啦
template <typename T>
inline string tostr(T t) {
    return std::to_string(t);
}

//将int, float, double等转为字符串, 相对tostr()较慢
template<typename T>
inline string convert_from(T t) {
    std::stringstream ss;
    ss << t;
    return ss.str();
}

//将字符串转为int, float, double等. 注意float/double有精度问题
template <class T>
void convert_to(T &value, const string &s) {
    std::stringstream ss(s);
    ss >> value;
}

template <class T>
T convert_to(const string &s) {
    T value;
    std::stringstream ss(s);
    ss >> value;
    return value;
}

//大小写转换
string tolower(const string &input) {
    string s(input);
    transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

string toupper(const string &input) {
    string s(input);
    transform(s.begin(), s.end(), s.begin(), ::toupper);
    return s;
}

//移除空格
string trim(const string &s) {
    string::const_iterator it = s.begin();
    while (it != s.end() && ::isspace(*it))
        it++;

    string::const_reverse_iterator rit = s.rbegin();
    while (rit.base() != it && ::isspace(*rit))
        rit++;

    return string(it, rit.base());
}

string ltrim(const string &s) {
    string::const_iterator it = s.begin();
    while (it != s.end() && ::isspace(*it))
        it++;
    return string(it, s.end());
}

string rtrim(const string &s) {
    string::const_reverse_iterator rit = s.rbegin();
    while (rit != s.rend() && ::isspace(*rit))
        rit++;
    return string(s.begin(), rit.base());
}

//左右
string left(const string &str, size_t count) {
    return str.substr(0, count);
}

string right(const string &str, size_t count) {
    count = std::min(count, str.size());
    return str.substr(str.size() - count);
}

string mid(const string &str, size_t first, size_t count = string::npos) {
    if (first >= str.size())
        return string();
    return str.substr(first, count);
}

//Removes n characters from the end of the string.
//If n is greater than or equal to size(), the result is an empty string.
void chop(string &str, size_t n) {
    int pos = str.size() - n;
    if (pos > 0) {
        str.erase(pos, string::npos);
    } else {
        str.clear();
    }
}

//将@subject中所有的@search替换为@after. from http://www.zedwood.com/article/cpp-stringutils-function
string replace_all(const string& subject, const string& search, const string& after) {
    string str = subject;
    size_t pos = 0;
    while ((pos = str.find(search, pos)) != string::npos) {
        str.replace(pos, search.length(), after);
        pos += after.length();
    }
    return str;
}

///将@subject中所有的@search替换为@replace, 返回一个新字符串, ### 返回值必需用free()来删除 ###
//from http://www.binarytides.com/str_replace-for-c/
char *str_replace(char *subject, char *search, char *replace) {
    char  *p = NULL, *old = NULL, *new_subject = NULL;
    int c = 0;
    int search_size = strlen(search);

    //Count how many occurences
    for (p = strstr(subject, search); p != NULL; p = strstr(p + search_size, search)) {
        c++;
    }

    //Final size
    c = (strlen(replace) - search_size)*c + strlen(subject);
    //New subject with new size
    new_subject = (char*)malloc(c);
    //Set it to blank
    new_subject[0] = '\0';

    //The start position
    old = subject;
    for (p = strstr(subject, search); p != NULL; p = strstr(p + search_size, search)) {
        //move ahead and copy some text from original subject , from a certain position
        strncpy(new_subject + strlen(new_subject), old, p - old);
        //move ahead and copy the replacement text
        strcpy(new_subject + strlen(new_subject), replace);
        //The new start position after this search match
        old = p + search_size;
    }

    //Copy the part after the last search match
    strcpy(new_subject + strlen(new_subject), old);
    return new_subject;
}

//@subject是否包含@search
bool contains(const string& subject, const string& search) {
    return subject.find(search) != string::npos;
}

/*
 * 拆分C++ string字符串
 * tokens     -- 以vector返回拆后的各个值 
 * str        -- 待拆分的字符串
 * delimiters -- str中的分隔符，如："\n" "\t" "|||"
 */
std::vector<string>& split(std::vector<string>& tokens, 
                           const string& str,
                           const string& delimiters,
                           bool skip_empty = false
                           )
{
#if 0  //这个实现, @delimiters 可以是多个字符，任意一个字符都是分隔符
    // Skip delimiters at beginning.
    string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    string::size_type pos = str.find_first_of(delimiters, lastPos);

    while (string::npos != pos || string::npos != lastPos) {
        string tmp = str.substr(lastPos, pos - lastPos);
        if (!skip_empty || !tmp.empty()) {
            // Found a token, add it to the vector.
            tokens.push_back(tmp);
        }
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }
    return tokens;
#endif

    size_t pos1 = 0;
    size_t pos2 = str.find(delimiters, 0);

    while (string::npos != pos2) {
        string tmp = str.substr(pos1, pos2 - pos1);
        if (!skip_empty || !tmp.empty()) {
            tokens.push_back(tmp);
        }
        pos1 = pos2 + delimiters.size();
        pos2 = str.find(delimiters, pos1);
    }

    if (pos1 < str.length()) {
        tokens.push_back(str.substr(pos1));
    }
}

std::vector<string> split(const string& str,
                          const string& delimiters,
                          bool skip_empty = false
                         )
{
    vector<string> v;
    split(v, str, delimiters, skip_empty);
    return v;
}

//字符串格式化, 类似sprintf, 但不需要先分派缓冲区
string format(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    size_t size = vsnprintf(NULL, 0, fmt, ap) + 1; // Extra space for '\0'

    string str;
    str.resize(size);
    vsnprintf((char*)str.data(), size, fmt, ap);  //warning:: c++ 11不安全

    va_end(ap);
    return str;
}

/**
* note: 这函数在某些情况下与Linux的basename函数返回值不同
path         dirname    basename
"/usr/lib"    "/usr"    "lib"       ok
"/usr/"       "/"       "usr"       X   不同, 返回:  /usr   ""
"usr"         "."       "usr"       ok  
"/"           "/"       "/"         ok 
"."           "."       "."         ok
".."          "."       ".."        ok
* @return The basename of the path
*/
std::string basename(const string& path, const char* sep="/\\")
{
    if (path == "/")
        return path;
    const size_t lastSlash = path.find_last_of(sep);
    if (lastSlash == std::string::npos)
        return path;

    return path.substr(lastSlash+1);
}

/**
 * note: 这函数在某些情况下与Linux的basename函数返回值不同
 * @return The directory name of the path
 */
std::string dirname(const string& path, const char* sep="/\\")
{
    if (path == "/")
        return path;
    const size_t lastSlash = path.find_last_of(sep);
    if (lastSlash == std::string::npos)
        return ".";

    return path.substr(0, lastSlash);
}

/*
* 将二进制转为16进制的字符. 一个byte为2个字符
*/
string bin2hex(const unsigned char *data, int len) {
    static const char hex[] = "0123456789ABCDEF";
    std::string out;
    out.reserve(len * 2 + 1);
    for (int i = 0; i < len; ++i) {
        unsigned char c = data[i];
        out += hex[c >> 4];
        out += hex[c & 0xf];
    }
    return out;
}

inline string bin2hex(const string &s) {
    return bin2hex((unsigned char*)s.data(), s.size());
}

inline int hex_to_bin(char ch) {
    if ((ch >= '0') && (ch <= '9'))
        return ch - '0';
    if ((ch >= 'a') && (ch <= 'f'))
        return ch - 'a' + 10;
    if ((ch >= 'A') && (ch <= 'F'))
        return ch - 'A' + 10;
    return -1;
}

/*
* 将16进制字符串转为二进制值. 2个字符合成一个byte。
* 注意：传入的字符串必需是0-9, a-f, A-F, 否则返回的结果是不对的
*/
string hex2bin(const char *data, int len) {
    string out;
    out.reserve(len / 2 + 2);
    if (len & 1 > 0) { // odd, 最前面一个字节算1个
        out += (char)hex_to_bin(*data++);
        len -= 1;
    }
    while (len > 0) {
        out += (char)(hex_to_bin(*data++) << 4 | hex_to_bin(*data++));
        len -= 2;
    }
    return out;
}

inline string hex2bin(const string & s) {
    return hex2bin(s.data(), s.size());
}

/*
* 像python的join, 把一个容器中的字符串合起来
* @param t      容器对象
* @param sep    分隔符
*/
template<typename T>
string join(const T& t, const string& sep="") {
    string ret;
    for (auto & e: t) {
        ret += e;
        ret += sep;
    }
    if (!ret.empty() && !sep.empty()) {
        ret.erase(ret.size() - sep.size());
    }
    return ret;
}

///////////////////////////////////////////////////////////////////////////////
//几个C语言的帮助函数
/*
* Copy src to string dst of size siz.  At most siz-1 characters
* will be copied.  Always NUL terminates (unless siz == 0).
* Returns strlen(src); if retval >= siz, truncation occurred.
*/
size_t strlcpy(char *dst, const char *src, size_t siz) 
{
    char *d = dst;
    const char *s = src;
    size_t n = siz;

    /* Copy as many bytes as will fit */
    if (n != 0) {
        while (--n != 0) {
            if ((*d++ = *s++) == '\0')
                break;
        }
    }

    /* Not enough room in dst, add NUL and traverse rest of src */
    if (n == 0) {
        if (siz != 0)
            *d = '\0';        /* NUL-terminate dst */
        while (*s++)
            ;
    }

    return(s - src - 1);    /* count does not include NUL */
}

/*
* Appends src to string dst of size siz (unlike strncat, siz is the
* full size of dst, not space left).  At most siz-1 characters
* will be copied.  Always NUL terminates (unless siz <= strlen(dst)).
* Returns strlen(src) + MIN(siz, strlen(initial dst)).
* If retval >= siz, truncation occurred.
*/
size_t strlcat(char *dst, const char *src, size_t siz) 
{
    char *d = dst;
    const char *s = src;
    size_t n = siz;
    size_t dlen;

    /* Find the end of dst and adjust bytes left but don't go past end */
    while (n-- != 0 && *d != '\0')
        d++;
    dlen = d - dst;
    n = siz - dlen;

    if (n == 0)
        return(dlen + strlen(s));
    while (*s != '\0') {
        if (n != 1) {
            *d++ = *s;
            n--;
        }
        s++;
    }
    *d = '\0';

    return (dlen + (s - src));    /* count does not include NUL */
}

/* this is like vsnprintf but the 'n' limit does not include
the terminating null. So if you have a 1024 byte buffer then
pass 1023 for n */
static int vslprintf(char *str, int n, char *format, va_list ap)
{
    int ret = vsnprintf(str, n, format, ap);
    if (ret > n || ret < 0) {
        str[n] = 0;
        return -1;
    }
    str[ret] = 0;
    return ret;
}

/* this is like snprintf but the 'n' limit does not include
the terminating null. So if you have a 1024 byte buffer then
pass 1023 for n */
int slprintf(char *str, int n, char *format, ...) {
    va_list ap;
    int ret;

    va_start(ap, format);
    ret = vslprintf(str, n, format, ap);
    va_end(ap);
    return ret;
}

/*
* Find the first occurrence of find in s, where the search is limited to the
* first slen characters of s.
*/
char* strnstr(const char *s, const char *find, size_t slen)
{
    char c, sc;
    size_t len;

    if ((c = *find++) != '\0') {
        len = strlen(find);
        do {
            do {
                if ((sc = *s++) == '\0' || slen-- < 1)
                    return (NULL);
            } while (sc != c);
            if (len > slen)
                return (NULL);
        } while (strncmp(s, find, len) != 0);
        s--;
    }

    return ((char *)s);
}

//统一的实现strcasecmp/strncasecmp 实现，来自apple
int strcasecmp(const char *s1, const char *s2);
int strncasecmp(const char *s1, const char *s2, size_t n);

/*
* http://www.opensource.apple.com/source/Libc/Libc-167/string.subproj/strcasecmp.c
* This array is designed for mapping upper and lower case letter
* together for a case independent comparison.  The mappings are
* based upon ascii character sequences.
*/
static const unsigned char charmap[] = {
    u'\000', u'\001', u'\002', u'\003', u'\004', u'\005', u'\006', u'\007',
    u'\010', u'\011', u'\012', u'\013', u'\014', u'\015', u'\016', u'\017',
    u'\020', u'\021', u'\022', u'\023', u'\024', u'\025', u'\026', u'\027',
    u'\030', u'\031', u'\032', u'\033', u'\034', u'\035', u'\036', u'\037',
    u'\040', u'\041', u'\042', u'\043', u'\044', u'\045', u'\046', u'\047',
    u'\050', u'\051', u'\052', u'\053', u'\054', u'\055', u'\056', u'\057',
    u'\060', u'\061', u'\062', u'\063', u'\064', u'\065', u'\066', u'\067',
    u'\070', u'\071', u'\072', u'\073', u'\074', u'\075', u'\076', u'\077',
    u'\100', u'\141', u'\142', u'\143', u'\144', u'\145', u'\146', u'\147',
    u'\150', u'\151', u'\152', u'\153', u'\154', u'\155', u'\156', u'\157',
    u'\160', u'\161', u'\162', u'\163', u'\164', u'\165', u'\166', u'\167',
    u'\170', u'\171', u'\172', u'\133', u'\134', u'\135', u'\136', u'\137',
    u'\140', u'\141', u'\142', u'\143', u'\144', u'\145', u'\146', u'\147',
    u'\150', u'\151', u'\152', u'\153', u'\154', u'\155', u'\156', u'\157',
    u'\160', u'\161', u'\162', u'\163', u'\164', u'\165', u'\166', u'\167',
    u'\170', u'\171', u'\172', u'\173', u'\174', u'\175', u'\176', u'\177',
    u'\200', u'\201', u'\202', u'\203', u'\204', u'\205', u'\206', u'\207',
    u'\210', u'\211', u'\212', u'\213', u'\214', u'\215', u'\216', u'\217',
    u'\220', u'\221', u'\222', u'\223', u'\224', u'\225', u'\226', u'\227',
    u'\230', u'\231', u'\232', u'\233', u'\234', u'\235', u'\236', u'\237',
    u'\240', u'\241', u'\242', u'\243', u'\244', u'\245', u'\246', u'\247',
    u'\250', u'\251', u'\252', u'\253', u'\254', u'\255', u'\256', u'\257',
    u'\260', u'\261', u'\262', u'\263', u'\264', u'\265', u'\266', u'\267',
    u'\270', u'\271', u'\272', u'\273', u'\274', u'\275', u'\276', u'\277',
    u'\300', u'\301', u'\302', u'\303', u'\304', u'\305', u'\306', u'\307',
    u'\310', u'\311', u'\312', u'\313', u'\314', u'\315', u'\316', u'\317',
    u'\320', u'\321', u'\322', u'\323', u'\324', u'\325', u'\326', u'\327',
    u'\330', u'\331', u'\332', u'\333', u'\334', u'\335', u'\336', u'\337',
    u'\340', u'\341', u'\342', u'\343', u'\344', u'\345', u'\346', u'\347',
    u'\350', u'\351', u'\352', u'\353', u'\354', u'\355', u'\356', u'\357',
    u'\360', u'\361', u'\362', u'\363', u'\364', u'\365', u'\366', u'\367',
    u'\370', u'\371', u'\372', u'\373', u'\374', u'\375', u'\376', u'\377',
};

int strcasecmp(const char *s1, const char *s2)
{
    typedef unsigned char u_char;
    register const u_char *cm = charmap,
        *us1 = (const u_char *)s1,
        *us2 = (const u_char *)s2;

    while (cm[*us1] == cm[*us2++])
        if (*us1++ == '\0')
            return (0);
    return (cm[*us1] - cm[*--us2]);
}

int strncasecmp(const char *s1, const char *s2, size_t n)
{
    typedef unsigned char u_char;
    if (n != 0) {
        register const u_char *cm = charmap,
            *us1 = (const u_char *)s1,
            *us2 = (const u_char *)s2;

        do {
            if (cm[*us1] != cm[*us2++])
                return (cm[*us1] - cm[*--us2]);
            if (*us1++ == '\0')
                break;
        } while (--n != 0);
    }
    return (0);
}

DSTR_NAMESPACE_END

#endif
