#ifndef DES_H
#define DES_H

/*
* DES encrypt/decrypt, come from https://github.com/fffaraz/cppDES
* a little demo code:
```cpp
DES3_Cipher d3("abcde", 5);
std::string d3_data = "abcdefghijklmn1234567890";
auto dx1 = d3.encrypt(d3_data);
auto dx2 = d3.decrypt(dx1);
```
*/

#include <cstdint>
#include <string>

#define ui64 uint64_t
#define ui32 uint32_t
#define ui8  uint8_t

class DES
{
public:
    DES(ui64 key); //8字节的key
    ui64 des(ui64 block, bool mode);

    ui64 encrypt(ui64 block);
    ui64 decrypt(ui64 block);

    static ui64 encrypt(ui64 block, ui64 key);
    static ui64 decrypt(ui64 block, ui64 key);

protected:
    void keygen(ui64 key);

    ui64 ip(ui64 block);
    ui64 fp(ui64 block);

    void feistel(ui32 &L, ui32 &R, ui32 F);
    ui32 f(ui32 R, ui64 k);

private:
    ui64 sub_key[16]; // 48 bits each
};

class DES3
{
public:
    DES3(ui64 k1, ui64 k2, ui64 k3) :   //3个8字节的key组合成24字节
        des1(k1),
        des2(k2),
        des3(k3)
    {}

    ui64 encrypt(ui64 block) {
        return des3.encrypt(des2.decrypt(des1.encrypt(block)));
    }

    ui64 decrypt(ui64 block) {
        return des1.decrypt(des2.encrypt(des3.decrypt(block)));
    }

private:
    DES des1, des2, des3;
};

class DESCBC
{
public:
    DESCBC(ui64 key, ui64 iv) :
        des(key),
        iv(iv),
        last_block(iv)
    {}

    ui64 encrypt(ui64 block) {
        last_block = des.encrypt(block ^ last_block);
        return last_block;
    }

    ui64 decrypt(ui64 block) {
        ui64 result = des.decrypt(block) ^ last_block;
        last_block = block;
        return result;
    }

    void reset() {
        last_block = iv;
    }

private:
    DES des;
    ui64 iv;
    ui64 last_block;
};

//下面是我加的函数，方便调用

//https://github.com/yudansan/3des/blob/master/d3d/d3des.c
//https://github.com/robertdavidgraham/ferret/blob/master/src/crypto-des.c

typedef char ElemType;


static ui64 str_to_key(const unsigned char str[8])
{
    //typedef union { ui64 x; unsigned char ch[sizeof(ui64)]; } u;

    //u data;
    //unsigned char *key = data.ch;
    //key[0] = (unsigned char)(str[0] >> 1);
    //key[1] = (unsigned char)(((str[0] & 0x01) << 6) | (str[1] >> 2));
    //key[2] = (unsigned char)(((str[1] & 0x03) << 5) | (str[2] >> 3));
    //key[3] = (unsigned char)(((str[2] & 0x07) << 4) | (str[3] >> 4));
    //key[4] = (unsigned char)(((str[3] & 0x0F) << 3) | (str[4] >> 5));
    //key[5] = (unsigned char)(((str[4] & 0x1F) << 2) | (str[5] >> 6));
    //key[6] = (unsigned char)(((str[5] & 0x3F) << 1) | (str[6] >> 7));
    //key[7] = (unsigned char)(str[6] & 0x7F);
    //for (unsigned i = 0; i < 8; i++) {
    //    key[i] = (unsigned char)(key[i] << 1);
    //}
    //return data.x;

    ui64 x = 0;
    //bool sk[64];
    bool *sk = (bool*)&x;
    for (int i = 0; i < 8; i++)
        for (int j = 0; j<8; ++j)
            //sk[i * 8 + (7 - j)] = (str[i] >> j) & 1;
            sk[i * 8 + j] = ( (str[i] >> j) & 1 );
    return x;
}

template<typename T>
class Cipher
{
public:
    Cipher(const char *key, int key_len);  //DES, DES3
    Cipher(const char *key, int key_len, const char *iv, int iv_len=8); //DESCBC

    ~Cipher() {
        delete des_;
    }

    inline std::string encrypt(const std::string& data) {
        return encrypt(data.data(), data.length());
    }

    inline std::string decrypt(const std::string& data) {
        return decrypt(data.data(), data.length());
    }
    
    /* encrypt 
    * @param data: the data for encrypt
    * @param len:  data length
    */
    std::string encrypt(const char* data, ui64 len);

    /* decrypt
    * @param data: the data for decrypt
    * @param len:  data length
    */
    std::string decrypt(const char* data, ui64 len);

private:
    T *des_;
};

typedef Cipher<DES>     DES_Cipher;
typedef Cipher<DESCBC>  DESCBC_Cipher;
typedef Cipher<DES3>    DES3_Cipher;

//////////////////////////////////////////////////////////////////////////
template<>
Cipher<DES>::Cipher(const char *key, int key_len)
{
    ui64 uikey = 0;
    memcpy((char*)&uikey, key, key_len > 8 ? 8 : key_len);
    uikey = str_to_key((unsigned char*)&uikey);
    des_ = new DES(uikey);
}

template<>
Cipher<DES3>::Cipher(const char *key, int key_len)
{
    ui64 uikey[3] = {0};
    if (key_len <= 8) {
        memcpy((char*)&uikey[0], key, key_len);
        uikey[1] = uikey[2] = uikey[0];
    }
    else if (key_len <= 16) {
        memcpy((char*)&uikey[0], key, 8);
        memcpy((char*)&uikey[1], key + 8, key_len - 8);
        uikey[2] = uikey[0];
    }
    else {  // key_len > 16
        memcpy((char*)&uikey[0], key, 8);
        memcpy((char*)&uikey[1], key + 8, 8);
        memcpy((char*)&uikey[2], key + 16, (key_len - 16 > 8) ? 8 : (key_len - 16) );
    }
    des_ = new DES3(uikey[0], uikey[1], uikey[2]);
}

template<>
Cipher<DESCBC>::Cipher(const char *key, int key_len, const char *iv, int iv_len)
{
    ui64 uikey = 0, uiv = 0;
    memcpy((char*)&uikey, key, key_len > 8 ? 8: key_len);
    memcpy((char*)&uiv,   iv,  iv_len > 8 ? 8: iv_len);
    des_ = new DESCBC(uikey, uiv);
}

template<typename T>
std::string Cipher<T>::encrypt(const char* data, ui64 len) 
{
    ui64 buffer   = 0;
    ui64 block    = len / 8;
    const char *p = (const char*)data;

    std::string ret;
    ret.reserve(len + 8 + 1);
    for (ui64 i = 0; i < block; i++, p+=8) {
        memcpy(&buffer, p, 8);
        buffer = des_->encrypt(buffer);
        ret.append((char*)&buffer, sizeof(buffer));
    }

    // Amount of padding needed
    ui8 padding = 8 - (len % 8);

    // Padding cannot be 0 (pad full block)
    if (padding == 0)
        padding = 8;

    // Read remaining part of file
    buffer = (ui64) 0;
    if (padding != 8)
        memcpy((char*)&buffer, (const char*)p, 8 - padding);

    //about padding, please see：http://blog.csdn.net/alonesword/article/details/17385359
    //now I choose PKCS5
    memset((char*)&buffer + 8 - padding, padding, padding);
    buffer = des_->encrypt(buffer);

    ret.append((char*)&buffer, sizeof(buffer));
    return ret;
}

template<typename T>
std::string Cipher<T>::decrypt(const char* data, ui64 len) 
{
    if (len <= 0 || len % 8 != 0) {
        return std::string(); 
    }

    ui64 buffer   = 0;
    ui64 block    = len / 8;
    const char *p = (const char*)data;

    std::string ret;
    ret.reserve(len + 1);

    block--;
    for (ui64 i = 0; i < block; i++, p+=8) {
        memcpy(&buffer, p, 8);
        buffer = des_->decrypt(buffer);
        ret.append((char*)&buffer, sizeof(buffer));
    }

    // Read last line
    buffer = (ui64)0;
    memcpy((char*)&buffer, (const char*)p, 8);
    buffer = des_->decrypt(buffer);

    // Amount of padding on file
    const unsigned char *q = (const unsigned char*)&buffer;
    ui8 padding = q[7];
    if (padding < 8) {
        ui8 zero_count = 0;
        while (*q-- == '\0') {
            zero_count++;
        }
        if (zero_count > 0) {
            padding = zero_count;
        }
        if (padding != 8)
            ret.append((char*)&buffer, 8 - padding);
    } else {
        ret.append((char*)&buffer, 8);
    }

    return ret;
}


#define D_DES_HEADER_ONLY
#include "des.cpp"

#endif // DES_H
