
#ifndef _REGEX_UTIL_H_
#define _REGEX_UTIL_H_

#include <string>
#include <vector>
#include "deelx.h"

namespace re {

/*
 * python re.findall的c++实现, 这个函数@pattern中仅支持1个分组, 如果有多个分组的话也仅取第1个分组的值
 * re.findall(pattern, string, flags=0) 
 * Return all non-overlapping matches of pattern in string, as a list of strings. The string is scanned left-to-right, and matches are returned in the order found. If one or more groups are present in the pattern, return a list of groups; this will be a list of tuples if the pattern has more than one group. Empty matches are included in the result unless they touch the beginning of another match.
 * @param pattern  正则表达式
 * @param text     待匹配的文本
 * @param flags    暂不使用
 * @return 返回匹配的字符串列表
*/
std::vector<std::string> findall(const char* pattern, const char* text, int flags=0)
{
    std::vector<std::string> vec;
    CRegexpT<char> regexp(pattern);
    CContext *pContext = regexp.PrepareMatch(text);    //prepare

    //loop
    MatchResult result = regexp.Match(pContext);
    while (result.IsMatched()) {
        if (result.MaxGroupNumber() <= 0) {
            vec.emplace_back(text + result.GetStart(), result.GetEnd() - result.GetStart());
        } else {
            vec.emplace_back(text + result.GetGroupStart(1), result.GetGroupEnd(1) - result.GetGroupStart(1));
        }
        result = regexp.Match(pContext);   //get next
    }

    regexp.ReleaseContext(pContext);    //release
    return vec;
}

inline std::vector<std::string> findall(const std::string& pattern, const std::string& text, int flags=0)
{
    return findall(pattern.c_str(), text.c_str(), flags);
}


} //end of ---->namespace re {

#endif

