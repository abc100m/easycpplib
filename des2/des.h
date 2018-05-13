#ifndef _D_DES_H_
#define _D_DES_H_

#include <string>
#include <memory>
#include "mbedtls_des.h"


template<unsigned T>
class DES
{
public:
    DES(const char *key, int key_len = 0, const char *iv = NULL, int iv_len = 0) 
    {
        assert(NULL != key);
        mbedtls_des_init(&ctx_);

        if (NULL != iv) {
            if (0 == iv_len) {
                iv_len = strlen(iv);
            }
            if (iv_len > 0) {
                iv_ = std::string(iv, iv_len);
            }
        }

        if (0 == key_len) {
            key_len = strlen(key);
        }
        key_ = std::string(key, key_len);
    }

    DES(const std::string& key, const std::string& iv = "")
        :DES(key.c_str(), key.length(), iv.c_str(), iv.length())
    {}

    ~DES() {
        mbedtls_des_free(&ctx_);
    }
    DES(const DES &r) = delete;
    DES operator=(const DES &r) = delete;

public:
    /*
    * @brief DES/DES3加密
    * @param data 待加密的数据
    * @param len  数据长度
    * @return 返回加密后的数据, string.data()是数据, string.size()是长度
    */
    std::string encrypt(const char *data, int len);

    std::string encrypt(const std::string& data) {
        return encrypt(data.data(), data.length());
    }

    std::string decrypt(const char *data, int len, bool padding = true);
    std::string decrypt(const std::string& data, bool padding = true) {
        return decrypt(data.data(), data.length(), padding);
    }

private:
    mbedtls_des_context ctx_;
    std::string key_;
    std::string iv_;
};

typedef DES DES_Cipher;

//
//typedef DES<DES_Trait_DES>    DES_Cipher;
//typedef DES<DES_Trait_DES3>  DES3_Cipher;
//struct DES_Trait_DES {
//};
//struct DES_Trait_DES3 {
//};


std::string DES::encrypt(const char *src, int len, Int2Type<T>)
{
    std::string ret;
    if (len < 0) {
        return ret;
    }

    //about padding, please see：http://blog.csdn.net/alonesword/article/details/17385359
    //now I choose PKCS5
    int padding = 8 - len % 8;
    int new_len = len + padding;
    std::unique_ptr<unsigned char[]> padding_data(new unsigned char[new_len]);
    unsigned char* after_padding = padding_data.get();
    memcpy(after_padding, src, len);
    for (int i = 0; i < padding; i++) {
        after_padding[len + i] = padding;
    }

    ret.resize(new_len);

    //
    char use_key[8] = { 0 };
    memcpy(use_key, key_.data(), std::min(8u, key_.size()));
    mbedtls_des_setkey_enc(&ctx_, (const unsigned char*)use_key);
    if (!iv_.empty()) {
        unsigned char iv[8] = { 0 };
        memcpy(iv, (unsigned char*)iv_.data(), 8);
        mbedtls_des_crypt_cbc(&ctx_, MBEDTLS_DES_ENCRYPT, new_len, iv, after_padding, (unsigned char*)ret.data());
    } else {
        unsigned char* output = (unsigned char*)ret.data();
        unsigned char* input  = after_padding;
        while (new_len > 0) {
            mbedtls_des_crypt_ecb(&ctx_, input, output);
            input += 8;
            output += 8;
            new_len -= 8;
        }
    }

    return ret;
}

std::string DES::decrypt(const char *data, int len, bool padding)
{
    std::string ret;
    if (len <= 0 || len % 8 != 0) {
        return ret;
    }
    ret.resize(len);

    //
    int new_len = len;
    char use_key[8] = { 0 };
    memcpy(use_key, key_.data(), std::min(8u, key_.size()));
    mbedtls_des_setkey_dec(&ctx_, (const unsigned char*)use_key);
    if (!iv_.empty()) {
        unsigned char iv[8] = { 0 };
        memcpy(iv, (unsigned char*)iv_.data(), 8);
        mbedtls_des_crypt_cbc(&ctx_, MBEDTLS_DES_DECRYPT, new_len, iv, (const unsigned char*)data, (unsigned char*)ret.data());
    } else {
        unsigned char* output = (unsigned char*)ret.data();
        const unsigned char* input = (const unsigned char*)data;
        while (new_len > 0) {
            mbedtls_des_crypt_ecb(&ctx_, input, output);
            input += 8;
            output += 8;
            new_len -= 8;
        }
    }

    if (padding) {
        int pos = ret.size() - (int)ret.back();
        if (pos > 0) {
            ret.erase(pos, string::npos);
        } else {
            ret.clear();
        }
    }

    return ret;
}

#endif
