#ifndef _BASE64_H_
#define _BASE64_H_

/*base64编解码, 修改自libetpan-->https://github.com/dinhviethoa/libetpan/tree/master/src/data-types
*/

#include <stdlib.h>
#include <string>


#define BASE64_NAMESPACE_BEGIN    namespace base64 {  
#define BASE64_NAMESPACE_END      }

BASE64_NAMESPACE_BEGIN

#define CHAR64(c)  (((c) < 0 || (c) > 127) ? -1 : index_64[(c)])

static char index_64[128] = {
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,62, -1,-1,-1,63,
    52,53,54,55, 56,57,58,59, 60,61,-1,-1, -1,-1,-1,-1,
    -1, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,
    15,16,17,18, 19,20,21,22, 23,24,25,-1, -1,-1,-1,-1,
    -1,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,
    41,42,43,44, 45,46,47,48, 49,50,51,-1, -1,-1,-1,-1
};

static char basis_64[] =
   "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*base64编码, 编码后输出到@out_str, 成功返回0
*/
int encode_base64(const char * in, int len, std::string& out_str)
{
    if ((len > 0) && (in == NULL))
        return -1;

    int out_len = ((len + 2) / 3 * 4) + 1;
    out_str.resize(out_len);

    unsigned char oval;
    const unsigned char * uin = (const unsigned char *)in;

    char *tmp = (char*)out_str.data();
    while (len >= 3) {
        *tmp++ = basis_64[uin[0] >> 2];
        *tmp++ = basis_64[((uin[0] << 4) & 0x30) | (uin[1] >> 4)];
        *tmp++ = basis_64[((uin[1] << 2) & 0x3c) | (uin[2] >> 6)];
        *tmp++ = basis_64[uin[2] & 0x3f];
        uin += 3;
        len -= 3;
    }
    if (len > 0) {
        *tmp++ = basis_64[uin[0] >> 2];
        oval = (uin[0] << 4) & 0x30;
        if (len > 1) oval |= uin[1] >> 4;
        *tmp++ = basis_64[oval];
        *tmp++ = (len < 2) ? '=' : basis_64[(uin[1] << 2) & 0x3c];
        *tmp++ = '=';
    }

    *tmp = '\0';
    return 0;
}

/*base64解码, 解码后输出到@out_str, 成功返回0
*/
int decode_base64(const char *in, int len, std::string& out_str)
{
    if (in == NULL || len < 0) {
        return -1;
    }
    //https://github.com/roxlu/ofxStringEncoders/blob/master/src/lib/stringencoders/modp_b64.h
    //#define modp_b64_decode_len(A) (A / 4 * 3 + 2)
    int output_size = len / 4 * 3 + 2;
    out_str.resize(output_size + 1);

    int i, c1, c2, c3, c4;
    char *output, *start;
    output = start = (char*)out_str.data();

    if (in[0] == '+' && in[1] == ' ')
        in += 2;

    for (i = 0; i < (len / 4); i++) {
        c1 = in[0];
        c2 = in[1];
        c3 = in[2];
        c4 = in[3];
        if (CHAR64(c1) == -1 || CHAR64(c2) == -1 ||
            (c3 != '=' && CHAR64(c3) == -1) ||
            (c4 != '=' && CHAR64(c4) == -1)) {
            out_str.clear();
            return -1;
        }

        in += 4;
        *output++ = (CHAR64(c1) << 2) | (CHAR64(c2) >> 4);

        if (c3 != '=') {
            *output++ = ((CHAR64(c2) << 4) & 0xf0) | (CHAR64(c3) >> 2);
            if (c4 != '=') {
                *output++ = ((CHAR64(c3) << 6) & 0xc0) | CHAR64(c4);
            }
        }
    }

    *output = 0;
    out_str.resize(output - start);
    return 0;
}

/*base64编码, 返回编码后的字符串. 若失败则返回空字符串
*/
inline std::string encode_base64(const char * in, int len)
{
    std::string ret;
    encode_base64(in, len, ret);
    return ret;    
}

inline std::string encode_base64(const std::string& in)
{
    return encode_base64(in.c_str(), in.length());
}

/*base64解码, 返回解码后的字符串. 若失败则返回空字符串
*/
inline std::string decode_base64(const char *in, int len)
{
    std::string ret;
    decode_base64(in, len, ret);
    return ret;
}

inline std::string decode_base64(const std::string& in)
{
    return decode_base64(in.c_str(), in.length());
}

BASE64_NAMESPACE_END

#endif
