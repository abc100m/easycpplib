
rsa提取自 axtls,  官网：http://axtls.sourceforge.net/


直接 #include "crypto.h" 即可使用, 无需再链接.c代码.
在包含crypto.h前可以定义如下宏:
#define CONFIG_SSL_CERT_VERIFICATION        //用签名
#define CONFIG_SSL_GENERATE_X509_CERT       //用于加密

我再包装了下，#include "easy_rsa.h"

########################################################################
https://github.com/dsheets/axtls  比较旧了, 过时

axtls需要修改， 但在网上找到了这个, 稍微些有旧了：
    https://hackage.haskell.org/package/adb-0.1.0.0/src/Network/ADB/


https://github.com/cesanta/krypton  这个TLS库挺好，只需要2个文件

