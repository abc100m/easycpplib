#ifndef _UTILS_DATETIME_H_
#define _UTILS_DATETIME_H_

#include "config.h"
#include <time.h>
#include <string>
#include <stdio.h>
#include <ctype.h>

//for::   struct timeval
#ifdef WIN32
#   include <winsock2.h>
#   include <Windows.h>
#   include <string.h>
#else
#   include <sys/time.h>
#   include <time.h>
#endif

DTIME_NAMESPACE_BEGIN

using std::string;

//clock_t clock (void);
//使用 clock() 当前cpu时间, 获取 @start @end的时间间隔.  这个用于测试2段代码之间的耗时
double clock_interval_sec(clock_t start, clock_t end) {  //秒
    return (double)(end - start) / CLOCKS_PER_SEC;
}

double clock_interval_ms(clock_t start, clock_t end) {   //毫秒
    return (end - start) * 1000.0 / CLOCKS_PER_SEC;
}

void time_to_local(const time_t& t, struct tm *out) {
#if defined(WIN32) || defined(__MINGW32__)
    *out = *localtime(&t);
#else
    localtime_r(&t, out);
#endif
}

//t可以是time(0), 用于跨平台调用gmtime_r、localtime_r
struct tm timet_to_local(const time_t& t) {
    struct tm out;
    time_to_local(t, &out);
    return out;
}

void time_to_gmt(const time_t& t, struct tm *out) {
#if defined(WIN32) || defined(__MINGW32__)
    *out = *gmtime(&t);
#else
    gmtime_r(&t, out);
#endif
}

struct tm timet_to_gmt(const time_t& t) {
    struct tm out;
    time_to_gmt(t, &out);
    return out;
}

//#if defined _WIN32 || defined __CYGWIN__  || defined __MINGW32__
#if defined(WIN32) || defined(__MINGW32__) || defined(__STRICT_ANSI__)  //mingw-64不定义WIN32宏，good
//网上抄来的一份实现，用于跨平台。但这个代码没有 %z，不能处理 时区
char* strptime(const char *buf, const char *fmt, struct tm *tm);
#endif

#ifdef WIN32
struct timezone;
int gettimeofday(struct timeval * tp, struct timezone * tzp)
#endif

//获取系统当前时间, 这是gettimeofday的封装, 但返回的类名改为d_timeval
void sysdate(struct timeval *out) {
    gettimeofday(out, NULL);
}

/*
* 相对于 time(NULL)返回的秒数, 这个函数返回的是毫秒数
*/
long long time_ms() {
    struct timeval d;
    gettimeofday(&d, NULL);
    return (long long)d.tv_sec * 1000 + d.tv_usec / 1000;
}

//这个类是struct tm的封装
class Date_Time
{
public:
    static Date_Time now() {
        return Date_Time(time(0));
    }

    static Date_Time fromstr(const char* str, const char* fmt) {
        struct tm t;
        strptime(str, fmt, &t);
        return Date_Time(t);
    }

public:
    Date_Time(time_t t) {
        time_to_local(t, &tm_);
    }

    Date_Time(const struct tm& t) {
        tm_ = t;
    }

public:
    //yyyy-mm-dd
    string str_date() const {
        return str_format("%Y-%m-%d");
    }

    //hh:mi:ss
    string str_time() const {
        return str_format("%H:%M:%S");
    }

    //yyyy-mm-dd hh:mi:ss
    string str_datetime() const {
        return str_format("%Y-%m-%d %H:%M:%S");
    }

    //这是strftime()的封装, 不支持毫秒. 用 %z 可获取时区偏移
    string str_format(const char* fmt) const {
        char buf[64];
        strftime(buf, sizeof(buf), fmt, &tm_);
        return buf;
    }

    inline int year() const {
        return tm_.tm_year + 1900;
    }

    inline int month() const {
        return tm_.tm_mon + 1;
    }
    
    inline int day() const {
        return tm_.tm_mday;
    }

    inline int hour() const {
        return tm_.tm_hour;
    }

    inline int minute() const {
        return tm_.tm_min;
    }

    inline int second() const {
        return tm_.tm_sec;
    }

    //normalize之后这个值可能就不准了. see: mktime
    inline int day_of_week() const {
        return tm_.tm_wday;
    }

    //normalize之后这个值可能就不准了. see: mktime
    inline int day_of_year() const {
        return tm_.tm_yday;
    }

    void set_year(int y) {
        tm_.tm_year = y - 1900;
    }

    void set_month(int m) {
        tm_.tm_mon = m - 1;
    }

    void set_day(int d) {
        tm_.tm_mday = d;
    }

    void set_hour(int h) {
        tm_.tm_hour = h;
    }

    void set_minute(int m) {
        tm_.tm_min = m;
    }

    void set_second(int s) {
        tm_.tm_sec = s;
    }

    void set_date(int y, int m, int d) {
        tm_.tm_year = y - 1900;
        tm_.tm_mon = m - 1;
        tm_.tm_mday = d;
    }

    void set_time(int h, int m, int s) {
        tm_.tm_hour = h;
        tm_.tm_min = m;
        tm_.tm_sec = s;
    }
    
public:
    void add_year(int y) {
        tm_.tm_year += y;
        normalize();
    }

    void add_month(int m) {
        tm_.tm_mon += m;
        normalize();
    }

    void add_day(int d) {
        tm_.tm_mday += d;
        normalize();
    }

    void add_hour(int h) {
        tm_.tm_hour += h;
        normalize();
    }

    void add_minute(int mm) {
        tm_.tm_min += mm;
        normalize();
    }

    void add_second(int s) {
        tm_.tm_sec += s;
        normalize();
    }

public:
    time_t to_time_t() {
        return mktime(&tm_);
    }

    inline const struct tm& tm_t() const {
        return tm_;
    }

    inline struct tm& tm_t() {
        return tm_;
    };

    time_t normalize() {
        return mktime(&tm_);
    }

private:
    struct tm tm_;
};

//带微秒的时间. 注意析构函数不是virtual的, 不要用--> Date_Time *d = new Date_Time_us();
class Date_Time_us : public Date_Time
{
public:
    static Date_Time_us now() {
        timeval tv;
        gettimeofday(&tv, NULL);
        return Date_Time_us((time_t)tv.tv_sec, tv.tv_usec);
    }

public:
    Date_Time_us(const struct tm& t, long us) 
        : Date_Time(t)
        , us_(us)
    {}

    Date_Time_us(time_t t, long us)
        : Date_Time(t)
        , us_(us)
    {}

public:
    //设置微秒
    void set_us(long us) {
        us_ = us;
    }

    //获得毫秒
    inline long us() const {
        return us_;
    }

    //获取微秒
    inline long ms() const {
        return us_ / 1000;
    }

    //增强Date_Time::str_format, 支持%f 3位数的毫秒
    string str_format_ex(const char* fmt) const {
        const char *ms_fmt = "%f";
        const int ms_fmt_len = 2;

        //计算毫秒
        long ms = us_ / 1000;
        char ms_str[16];
        int  ms_str_len = ::snprintf(ms_str, sizeof(ms_str), "%03ld", ms);

        //将%f替换成毫秒
        string s(fmt);
        size_t pos = 0;
        while ((pos = s.find(ms_fmt, pos)) != string::npos) {
            s.replace(pos, ms_fmt_len, ms_str);
            pos += ms_str_len;
        }

        //格式化字符串
        const struct tm& t = tm_t();
        char buf[64];
        strftime(buf, sizeof(buf), s.c_str(), &t);

        return buf;
    }

    //yyyy-mm-dd hh:mi:ss.fff
    string str_datetime_ms() const {
        return str_format_ex("%Y-%m-%d %H:%M:%S.%f");
    }

private:
    long us_;   //微秒
};
//////////////////////////////////////////////////////////////////////////

#if 0   //这个代码是C 标准库的说明
struct tm {
    int tm_sec; /* 秒 – 取值区间为[0,59] */
    int tm_min; /* 分 - 取值区间为[0,59] */
    int tm_hour; /* 时 - 取值区间为[0,23] */
    int tm_mday; /* 一个月中的日期 - 取值区间为[1,31] */
    int tm_mon; /* 月份（从一月开始，0代表一月） - 取值区间为[0,11] */
    int tm_year; /* 年份，其值等于实际年份减去1900 */
    int tm_wday; /* 星期 – 取值区间为[0,6]，其中0代表星期天，1代表星期一，以此类推 */
    int tm_yday; /* 从每年的1月1日开始的天数 – 取值区间为[0,365]，其中0代表1月1日，1代表1月2日，以此类推 */
    int tm_isdst; /* 夏令时标识符，实行夏令时的时候，tm_isdst为正。不实行夏令时的进候，tm_isdst为0；不了解情况时，tm_isdst()为负。*/
};

double difftime(time_t time1, time_t time0);
time_t mktime(struct tm * timeptr); 
time_t time(time_t * timer);  //utc时间的秒数
size_t strftime(
    char *strDest,
    size_t maxsize,
    const char *format,
    const struct tm *timeptr
);

//世界标准时间（即格林尼治时间）
struct tm * gmtime_r(const time_t *timer);

//将日历时间转化为本地时间
struct tm * localtime_r(const time_t * timer);
#endif

// windows下 gettimeofday 的实现
#ifdef WIN32  
/* FILETIME of Jan 1 1970 00:00:00. */
static const unsigned __int64 epoch = UINT64CONST(116444736000000000);
//struct timeval {
//    long tv_sec;
//    long tv_usec;
//};

/* Provided for compatibility with code that assumes that
the presence of gettimeofday function implies a definition
of struct timezone. */
struct timezone {
    int tz_minuteswest; /* of Greenwich */
    int tz_dsttime;     /* type of dst correction to apply */
};

/*
* timezone information is stored outside the kernel so tzp isn't used anymore.
* Note: this function is not for Win32 high precision timing purpose. See
* elapsed_time().
*/
int gettimeofday(struct timeval * tp, struct timezone * tzp)
{
    FILETIME    file_time;
    SYSTEMTIME  system_time;
    ULARGE_INTEGER ularge;

    GetSystemTime(&system_time);
    SystemTimeToFileTime(&system_time, &file_time);
    ularge.LowPart = file_time.dwLowDateTime;
    ularge.HighPart = file_time.dwHighDateTime;

    tp->tv_sec = (long)((ularge.QuadPart - epoch) / 10000000L);
    tp->tv_usec = (long)(system_time.wMilliseconds * 1000);

    return 0;
}
#endif  //end of::  gettimeofday 


// 跨平台的 strptime 的实现, 注意这个代码没有处理时区的： %Z %z 
#if defined(WIN32) || defined(__MINGW32__) || defined(__STRICT_ANSI__)
//from http://www.xuebuyuan.com/1845916.html

static int conv_num(const char **buf, int *dest, int llim, int ulim)
{
    int result = 0;

    /* The limit also determines the number of valid digits. */
    int rulim = ulim;

    if (**buf < '0' || **buf > '9')
        return (0);

    do {
        result *= 10;
        result += *(*buf)++ - '0';
        rulim /= 10;
    } while ((result * 10 <= ulim) && rulim && **buf >= '0' && **buf <= '9');

    if (result < llim || result > ulim)
        return (0);

    *dest = result;
    return (1);
}

static int strncasecmp(const char *s1, const char *s2, size_t n) {
    if (n != 0) {
        typedef unsigned char u_char;
        const u_char *us1 = (const u_char *)s1;
        const u_char *us2 = (const u_char *)s2;

        do {
            if (tolower(*us1) != tolower(*us2))
                return (tolower(*us1) - tolower(*us2));
            if (*us1++ == '\0')
                break;
            us2++;
        } while (--n != 0);
    }
    return (0);
}

#define ALT_E			0x01
#define ALT_O			0x02
#define	LEGAL_ALT(x)	{ if (alt_format & ~(x)) return (0); }
#define TM_YEAR_BASE   (1970)

static const char *day[7] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday",
    "Friday", "Saturday"
};
static const char *abday[7] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};
static const char *mon[12] = {
    "January", "February", "March", "April", "May", "June", "July",
    "August", "September", "October", "November", "December"
};
static const char *abmon[12] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};
static const char *am_pm[2] = {
    "AM", "PM"
};

char* strptime(const char *buf, const char *fmt, struct tm *tm)
{
    char c;
    const char *bp;
    size_t len = 0;
    int alt_format, i, split_year = 0;

    bp = buf;

    while ((c = *fmt) != '\0') {
        /* Clear `alternate' modifier prior to new conversion. */
        alt_format = 0;

        /* Eat up white-space. */
        if (isspace(c)) {
            while (isspace(*bp))
                bp++;

            fmt++;
            continue;
        }

        if ((c = *fmt++) != '%')
            goto literal;


    again:		switch (c = *fmt++) {
    case '%':	/* "%%" is converted to "%". */
        literal :
            if (c != *bp++)
                return (0);
        break;

        /*
        * "Alternative" modifiers. Just set the appropriate flag
        * and start over again.
        */
    case 'E':	/* "%E?" alternative conversion modifier. */
        LEGAL_ALT(0);
        alt_format |= ALT_E;
        goto again;

    case 'O':	/* "%O?" alternative conversion modifier. */
        LEGAL_ALT(0);
        alt_format |= ALT_O;
        goto again;

        /*
        * "Complex" conversion rules, implemented through recursion.
        */
    case 'c':	/* Date and time, using the locale's format. */
        LEGAL_ALT(ALT_E);
        if (!(bp = strptime(bp, "%x %X", tm)))
            return (0);
        break;

    case 'D':	/* The date as "%m/%d/%y". */
        LEGAL_ALT(0);
        if (!(bp = strptime(bp, "%m/%d/%y", tm)))
            return (0);
        break;

    case 'R':	/* The time as "%H:%M". */
        LEGAL_ALT(0);
        if (!(bp = strptime(bp, "%H:%M", tm)))
            return (0);
        break;

    case 'r':	/* The time in 12-hour clock representation. */
        LEGAL_ALT(0);
        if (!(bp = strptime(bp, "%I:%M:%S %p", tm)))
            return (0);
        break;

    case 'T':	/* The time as "%H:%M:%S". */
        LEGAL_ALT(0);
        if (!(bp = strptime(bp, "%H:%M:%S", tm)))
            return (0);
        break;

    case 'X':	/* The time, using the locale's format. */
        LEGAL_ALT(ALT_E);
        if (!(bp = strptime(bp, "%H:%M:%S", tm)))
            return (0);
        break;

    case 'x':	/* The date, using the locale's format. */
        LEGAL_ALT(ALT_E);
        if (!(bp = strptime(bp, "%m/%d/%y", tm)))
            return (0);
        break;

        /*
        * "Elementary" conversion rules.
        */
    case 'A':	/* The day of week, using the locale's form. */
    case 'a':
        LEGAL_ALT(0);
        for (i = 0; i < 7; i++) {
            /* Full name. */
            len = strlen(day[i]);
            if (strncasecmp(day[i], bp, len) == 0)
                break;

            /* Abbreviated name. */
            len = strlen(abday[i]);
            if (strncasecmp(abday[i], bp, len) == 0)
                break;
        }

        /* Nothing matched. */
        if (i == 7)
            return (0);

        tm->tm_wday = i;
        bp += len;
        break;

    case 'B':	/* The month, using the locale's form. */
    case 'b':
    case 'h':
        LEGAL_ALT(0);
        for (i = 0; i < 12; i++) {
            /* Full name. */
            len = strlen(mon[i]);
            if (strncasecmp(mon[i], bp, len) == 0)
                break;

            /* Abbreviated name. */
            len = strlen(abmon[i]);
            if (strncasecmp(abmon[i], bp, len) == 0)
                break;
        }

        /* Nothing matched. */
        if (i == 12)
            return (0);

        tm->tm_mon = i;
        bp += len;
        break;

    case 'C':	/* The century number. */
        LEGAL_ALT(ALT_E);
        if (!(conv_num(&bp, &i, 0, 99)))
            return (0);

        if (split_year) {
            tm->tm_year = (tm->tm_year % 100) + (i * 100);
        }
        else {
            tm->tm_year = i * 100;
            split_year = 1;
        }
        break;

    case 'd':	/* The day of month. */
    case 'e':
        LEGAL_ALT(ALT_O);
        if (!(conv_num(&bp, &tm->tm_mday, 1, 31)))
            return (0);
        break;

    case 'k':	/* The hour (24-hour clock representation). */
        LEGAL_ALT(0);
        /* FALLTHROUGH */
    case 'H':
        LEGAL_ALT(ALT_O);
        if (!(conv_num(&bp, &tm->tm_hour, 0, 23)))
            return (0);
        break;

    case 'l':	/* The hour (12-hour clock representation). */
        LEGAL_ALT(0);
        /* FALLTHROUGH */
    case 'I':
        LEGAL_ALT(ALT_O);
        if (!(conv_num(&bp, &tm->tm_hour, 1, 12)))
            return (0);
        if (tm->tm_hour == 12)
            tm->tm_hour = 0;
        break;

    case 'j':	/* The day of year. */
        LEGAL_ALT(0);
        if (!(conv_num(&bp, &i, 1, 366)))
            return (0);
        tm->tm_yday = i - 1;
        break;

    case 'M':	/* The minute. */
        LEGAL_ALT(ALT_O);
        if (!(conv_num(&bp, &tm->tm_min, 0, 59)))
            return (0);
        break;

    case 'm':	/* The month. */
        LEGAL_ALT(ALT_O);
        if (!(conv_num(&bp, &i, 1, 12)))
            return (0);
        tm->tm_mon = i - 1;
        break;

    case 'p':	/* The locale's equivalent of AM/PM. */
        LEGAL_ALT(0);
        /* AM? */
        if (strcasecmp(am_pm[0], bp) == 0) {
            if (tm->tm_hour > 11)
                return (0);

            bp += strlen(am_pm[0]);
            break;
        }
        /* PM? */
        else if (strcasecmp(am_pm[1], bp) == 0) {
            if (tm->tm_hour > 11)
                return (0);

            tm->tm_hour += 12;
            bp += strlen(am_pm[1]);
            break;
        }

        /* Nothing matched. */
        return (0);

    case 'S':	/* The seconds. */
        LEGAL_ALT(ALT_O);
        if (!(conv_num(&bp, &tm->tm_sec, 0, 61)))
            return (0);
        break;

    case 'U':	/* The week of year, beginning on sunday. */
    case 'W':	/* The week of year, beginning on monday. */
        LEGAL_ALT(ALT_O);
        /*
        * XXX This is bogus, as we can not assume any valid
        * information present in the tm structure at this
        * point to calculate a real value, so just check the
        * range for now.
        */
        if (!(conv_num(&bp, &i, 0, 53)))
            return (0);
        break;

    case 'w':	/* The day of week, beginning on sunday. */
        LEGAL_ALT(ALT_O);
        if (!(conv_num(&bp, &tm->tm_wday, 0, 6)))
            return (0);
        break;

    case 'Y':	/* The year. */
        LEGAL_ALT(ALT_E);
        if (!(conv_num(&bp, &i, 0, 9999)))
            return (0);

        tm->tm_year = i - TM_YEAR_BASE;
        break;

    case 'y':	/* The year within 100 years of the epoch. */
        LEGAL_ALT(ALT_E | ALT_O);
        if (!(conv_num(&bp, &i, 0, 99)))
            return (0);

        if (split_year) {
            tm->tm_year = ((tm->tm_year / 100) * 100) + i;
            break;
        }
        split_year = 1;
        if (i <= 68)
            tm->tm_year = i + 2000 - TM_YEAR_BASE;
        else
            tm->tm_year = i + 1900 - TM_YEAR_BASE;
        break;

        /*
        * Miscellaneous conversions.
        */
    case 'n':	/* Any kind of white-space. */
    case 't':
        LEGAL_ALT(0);
        while (isspace(*bp))
            bp++;
        break;


    default:	/* Unknown/unsupported conversion. */
        return (0);
    }  //end of switch


    }  //end of while

    /* LINTED functional specification */
    return ((char *)bp);
}
#endif // end of :: strptime

DTIME_NAMESPACE_END

#endif
