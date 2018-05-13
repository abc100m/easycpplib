#ifndef _EASY_RSA_H_
#define _EASY_RSA_H_

#include <string>

#define CONFIG_SSL_CERT_VERIFICATION        //用签名
#define CONFIG_SSL_GENERATE_X509_CERT       //用于加密
#include "crypto.h"
#include "../strutil.h" //依赖我的这个字符串库:: hex2bin

RSA_NAMESPACE_BEGIN

/*
* 这个头文件是对RSA的封装，简单易用。还不支持从PEM中读入文件
* 从PEM中读入文件可从 https://github.com/cesanta/krypton 中找要相关代码
*
* RSA的相关知识：：
* (N,e)是公钥，(N,d)是私钥
* RSA密钥计算工具--> http://www-cs-students.stanford.edu/~tjw/jsbn/rsa2.html
*/

/*
* RSA公钥加密. 这个库的padding方式是pkcs1.5, 写死的，加密的数据必须比密钥小11
* @param  hex_n     16进制字符串表达的公钥N, 512/1024/2048位
* @param  hex_n_len @hex_n的长度
* @param  hex_e     16进制字符串表达的公钥e, 一般是3个字节长即5或6个字符
* @param  data      待加密的数据
* @param  data_len  数据长度   最大值：@hex_n的字节数 - 11
* @return 成功返回加密后的数据, 通过s.data(), s.size()可读到数据；失败返回空
*/
std::string ersa_encrypt(
        const char *hex_n, int hex_n_len, 
        const char *hex_e, int hex_e_len,
        const char *data,  int data_len
        )
{
    RSA_CTX *rsa_ctx = NULL;
    auto rsa_n1 = NS_DSTR::hex2bin(hex_n, hex_n_len);
    auto rsa_e1 = NS_DSTR::hex2bin(hex_e, hex_e_len);
    RSA_pub_key_new(&rsa_ctx, (unsigned char*)rsa_n1.data(), rsa_n1.size(), 
        (unsigned char*)rsa_e1.data(), rsa_e1.size());

    string out(rsa_ctx->num_octets + 1, '\0');
    int r = RSA_encrypt(rsa_ctx, (unsigned char*)data, data_len, (unsigned char*)out.data(), 0);
    if (r < 0) {
        out.clear();
    } else {
        out.resize(r);
    }
    RSA_free(rsa_ctx);
    return out;
}

inline std::string ersa_encrypt(const std::string& hex_n, const std::string& hex_e, const std::string& data)
{
    return ersa_encrypt(hex_n.data(), hex_n.size(), hex_e.data(), hex_e.size(), data.data(), data.size());
}

/*
* RSA私钥解密
* @param  hex_n     16进制字符串表达的公钥N, 512/1024/2048位
* @param  hex_n_len @hex_n的长度
* @param  hex_e     16进制字符串表达的公钥e, 一般是3个字节长即5或6个字符
* @param  data      待解密的数据
* @param  data_len  数据长度
* @return 成功返回解密后的数据，通过s.data(), s.size()可读到数据；失败返回空
*/
std::string ersa_decrypt(
        const char *hex_n, int hex_n_len, 
        const char *hex_e, int hex_e_len,
        const char *hex_d, int hex_d_len,
        const char *data,  int data_len 
        )
{
    auto rsa_n1 = NS_DSTR::hex2bin(hex_n, hex_n_len);
    auto rsa_e1 = NS_DSTR::hex2bin(hex_e, hex_e_len);
    auto rsa_d1 = NS_DSTR::hex2bin(hex_d, hex_d_len);

    RSA_CTX *rsa_ctx = NULL;
    RSA_priv_key_new(&rsa_ctx, 
            (unsigned char*)rsa_n1.data(), rsa_n1.size(),
            (unsigned char*)rsa_e1.data(), rsa_e1.size(),
            (unsigned char*)rsa_d1.data(), rsa_d1.size()
          );

    int r = -1;
    string out(rsa_ctx->num_octets + 1, '\0');
    if (data_len >= rsa_ctx->num_octets) {
        r = RSA_decrypt(rsa_ctx, (unsigned char*)data, (unsigned char*)out.data(), out.size(), 1);
    }

    if (r < 0) {
        out.clear();
    } else {
        out.resize(r);
    }
    RSA_free(rsa_ctx);
    return out;
}

inline std::string ersa_decrypt(
        const std::string& hex_n, const std::string& hex_e, 
        const std::string& hex_d, const std::string& data
        )
{
    return ersa_decrypt(hex_n.data(), hex_n.size(), hex_e.data(), hex_e.size(), 
        hex_d.data(), hex_d.size(), data.data(), data.size());
}


/*
* RSA私钥签名. 这个库的padding方式是pkcs1.5, 写死的，签名的数据必须比密钥小11
* 注意：签名时的padding是固定值；而加密时的padding是随机值。所以相同的数据签名多次得到同一个结果
* @param  hex_n     16进制字符串表达的公钥N, 512/1024/2048位
* @param  hex_n_len @hex_n的长度
* @param  hex_e     16进制字符串表达的公钥e, 一般是3个字节长即5或6个字符
* @param  data      待签名的数据
* @param  data_len  数据长度
* @return 成功返回签名后的数据, 通过s.data(), s.size()可读到数据；失败返回空
*/
std::string ersa_sign(
        const char *hex_n, int hex_n_len, 
        const char *hex_e, int hex_e_len,
        const char *hex_d, int hex_d_len,
        const char *data,  int data_len 
        )
{
    auto rsa_n1 = NS_DSTR::hex2bin(hex_n, hex_n_len);
    auto rsa_e1 = NS_DSTR::hex2bin(hex_e, hex_e_len);
    auto rsa_d1 = NS_DSTR::hex2bin(hex_d, hex_d_len);

    RSA_CTX *rsa_ctx = NULL;
    RSA_priv_key_new(&rsa_ctx, 
            (unsigned char*)rsa_n1.data(), rsa_n1.size(),
            (unsigned char*)rsa_e1.data(), rsa_e1.size(),
            (unsigned char*)rsa_d1.data(), rsa_d1.size()
          );

    string out(rsa_ctx->num_octets + 1, '\0');
    int r = RSA_encrypt(rsa_ctx, (unsigned char*)data, data_len, (unsigned char*)out.data(), 1);
    if (r >= 0) {
        out.resize(r);
    } else {
        out.clear();
    }
    RSA_free(rsa_ctx);
    return out;
}

inline std::string ersa_sign(
        const std::string& hex_n, const std::string& hex_e, 
        const std::string& hex_d, const std::string& data
        )
{
    return ersa_sign(hex_n.data(), hex_n.size(), hex_e.data(), hex_e.size(), 
        hex_d.data(), hex_d.size(), data.data(), data.size());
}

/*
* RSA公钥核查签名数据.
* @param  hex_n     16进制字符串表达的公钥N, 512/1024/2048位
* @param  hex_n_len @hex_n的长度
* @param  hex_e     16进制字符串表达的公钥e, 一般是3个字节长即5或6个字符
* @param  data      待核查的数据
* @param  data_len  数据长度   最大值：@hex_n的字节数 - 11
* @return 成功返回解密后待核查的数据, 通过s.data(), s.size()可读到数据；失败返回空
*/
std::string ersa_verify(
        const char *hex_n, int hex_n_len,
        const char *hex_e, int hex_e_len,
        const char *data,  int data_len
        )
{
    RSA_CTX *rsa_ctx = NULL;
    auto rsa_n1 = NS_DSTR::hex2bin(hex_n, hex_n_len);
    auto rsa_e1 = NS_DSTR::hex2bin(hex_e, hex_e_len);
    RSA_pub_key_new(&rsa_ctx, (unsigned char*)rsa_n1.data(), rsa_n1.size(), 
        (unsigned char*)rsa_e1.data(), rsa_e1.size());

    string out(rsa_ctx->num_octets + 1, '\0');
    int r = -1;
    if (data_len >= rsa_ctx->num_octets) {
        r = RSA_decrypt(rsa_ctx, (unsigned char*)data, (unsigned char*)out.data(), out.size(), 0);
    }
    if (r < 0) {
        out.clear();
    } else {
        out.resize(r);
    }
    RSA_free(rsa_ctx);
    return out;
}

inline std::string ersa_verify(const std::string& hex_n, const std::string& hex_e, const std::string& data)
{
    return ersa_verify(hex_n.data(), hex_n.size(), hex_e.data(), hex_e.size(), data.data(), data.size());
}

RSA_NAMESPACE_END

#endif
