#ifndef _D_URI_URL_H_
#define _D_URI_URL_H_

/*
 * ****comre from:: https://github.com/netmindms/urlparser
 * Url.h
 *  Created on: Nov 25, 2014
 *  Author: netmind
 */

#include <unordered_map>
#include <vector>
#include <string>
#include "uri_codec.h"

#ifndef NAMESAPCE_URI_BEGIN
#define NAMESAPCE_URI_BEGIN namespace uri {
#endif 

#ifndef NAMESPACE_URI_END
#define NAMESPACE_URI_END   }
#endif

NAMESAPCE_URI_BEGIN

using std::string;
using std::vector;
using std::unordered_map;

class Url {
public:
    int parse(const string& rawurl);

	string scheme;
    string username;
    string passwrod;
	string hostname;
	string port;
	string path;
	string query;
	string fragment;
};

/*
* 用于解析url
* @param urlstr 待解析的url字符串. 必须包含scheme. 例 http://www.github.com   而www.github.com是不能解析的!
* @param ok     用于返回成功与否, 默认为NULL则不关心返回值
* @return 返回Url对象
*/
Url parse_url(const string& urlstr, bool *ok = NULL) {
    Url url;
    int n = url.parse(urlstr);
    if (ok) {
        *ok = (0 == n);
    }
    return url;
}

/*
* 先自动decode，然后再解析url. 如果解析失败则自动加上http://再解析一次, 所以必然是成功解析的
*/
Url parse_url_smart(const string& urlstr) {
    auto data = uri_decode(urlstr);
    Url url;
    int n = url.parse(data);
    if (0 != n) {
        data.insert(0, "http://");
        url.parse(data);
    }
    return url;
}

/*
* 解析key-value
* @param kvmap key-value map
* @param rawstr 待解析的数据
* @param strict true时只有key-value都非空才存入结果; false时只要key-value任意值非空即存入结果
* @return 成功返回key-value的元素个数, -1为失败
*/
size_t parse_query_string(unordered_map<string, string> *kvmap, const string& rawstr, bool strict = false);

typedef struct {
    string key;
    string val;
} query_kv_t;
size_t parse_query_string(vector<query_kv_t> *kvvec, const string& rawstr, bool strict = false);
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define CHECK_LEN_END(POS, LEN) if(POS>=LEN) {_url_errorno=100;goto __PARSE_END;}
#define WALK_SP(POS, LEN, BUF) for(;POS<LEN && BUF[POS]==' ';POS++)
#define WALK_UNTIL(POS, LEN, BUF, DELC) for(;POS<LEN && BUF[POS]!=DELC;POS++)
#define WALK_UNTIL2(POS, LEN, BUF, DELI1, DELI2) for(;POS<LEN && BUF[POS]!=DELI1 && BUF[POS]!=DELI2 ;POS++)
#define WALK_UNTIL3(POS, LEN, BUF, DELI1, DELI2, DELI3) for(;POS<LEN && BUF[POS]!=DELI1 && BUF[POS]!=DELI2 && BUF[POS]!=DELI3;POS++)
#define CHECK_REMAIN_END(POS, LEN, REQ_LEN) if(LEN-POS < REQ_LEN) {_url_errorno=100; goto __PARSE_END; }
#define WALK_CHAR(POS, BUF, DELI) if(BUF[POS++] != DELI) goto __PARSE_END


int Url::parse(const string& rawurl) {
	int _url_errorno = 0;
	const char *str = rawurl.c_str();

	size_t pos, len, at_pos;
	int scheme_pos, username_pos, host_pos, port_pos, path_pos, param_pos, tag_pos;
	pos = 0;
	len = rawurl.size();
	WALK_SP(pos, len, str); // remove preceding spaces.
	if (str[pos] == '/') {
        goto __PARSE_USERNAME;
	}

	// start protocol scheme
	scheme_pos = pos;
	WALK_UNTIL(pos, len, str, ':');
	CHECK_LEN_END(pos, len);
	scheme = rawurl.substr(scheme_pos, pos - scheme_pos);
	CHECK_REMAIN_END(pos, len, 3);
	WALK_CHAR(pos, str, ':');
	WALK_CHAR(pos, str, '/');

    __PARSE_USERNAME:
    WALK_CHAR(pos, str, '/');
    username_pos = pos;
    at_pos = pos;
    WALK_UNTIL3(at_pos, len, str, '@', '/', '?');
    if (at_pos < len && str[at_pos] == '@') {
        WALK_UNTIL2(pos, len, str, '@', ':');
        this->username = rawurl.substr(username_pos, pos - username_pos);
        if (str[pos] == ':') {   
            this->passwrod = rawurl.substr(pos + 1, at_pos - pos - 1);
        }
        pos = at_pos + 1;
    }

	// start host address
	__PARSE_HOST:
	host_pos = pos;
	WALK_UNTIL3(pos, len, str, ':', '/', '?');
	if (pos < len) {
		hostname = rawurl.substr(host_pos, pos - host_pos);
		if (str[pos] == ':')
			goto __PARSE_PORT;
		if (str[pos] == '/')
			goto __PARSE_PATH;
		if (str[pos] == '?')
			goto __PARSE_PARAM;
	} else {
		hostname = rawurl.substr(host_pos, pos - host_pos);
	}

	__PARSE_PORT:
	WALK_CHAR(pos, str, ':');
	port_pos = pos;
	WALK_UNTIL2(pos, len, str, '/', '?');
	port = rawurl.substr(port_pos, pos - port_pos);
	CHECK_LEN_END(pos, len);
	if (str[pos] == '?')
		goto __PARSE_PARAM;

	__PARSE_PATH: 
    path_pos = pos;
	WALK_UNTIL(pos, len, str, '?');
	path = rawurl.substr(path_pos, pos - path_pos);
	CHECK_LEN_END(pos, len);

	__PARSE_PARAM:
	WALK_CHAR(pos, str, '?');
	param_pos = pos;
	WALK_UNTIL(pos, len, str, '#');
	query = rawurl.substr(param_pos, pos - param_pos);
	CHECK_LEN_END(pos, len);

	// start parsing fragment
	WALK_CHAR(pos, str, '#');
	tag_pos = pos;
	fragment = rawurl.substr(tag_pos, len - tag_pos);

    __PARSE_END: 
    return _url_errorno;
}

typedef int (*__kv_callback)(void* list, const string& k, const string& v);
size_t parseKeyValue(const string& rawstr, __kv_callback kvcb, void* obj, bool strict) {

	int _url_errorno = 0;
	const char *str = rawstr.c_str();
	size_t pos, len, item_len;
	pos = 0;
	len = rawstr.size();

	string key;
	size_t key_pos;
	WALK_SP(pos, len, str);
	CHECK_LEN_END(pos, len);
	key_pos = pos;
	item_len = 0;
	for(;;) {
		WALK_UNTIL2(pos, len, str, '=', '&');
		if (pos >= len || str[pos] == '&') {
			// Be careful for boundary check error to be caused. !!!
			// *** Do not access str[] any more in this block. !!!

			auto val = rawstr.substr(key_pos, pos-key_pos);
			if (strict) {
				if (!key.empty() && !val.empty()) {
					kvcb(obj, key, val);
					item_len++;
				}
			}
            else if(!(key.empty() && val.empty())){
				kvcb(obj, key, val);
				item_len++;
			}

			key.clear();
			if(pos >= len) goto __PARSE_END;
			pos++;
			key_pos = pos;
		}
		else if(str[pos] == '=') {
			key = rawstr.substr(key_pos, pos-key_pos);
			pos++;
			key_pos = pos;
		}
	}
	__PARSE_END:
	if(_url_errorno != 0 )
		return -1;
	return item_len;
}

static inline int __kv_callback_map(void* list, const string& k, const string& v) {
	auto *map = (unordered_map<string, string>*)list;
	(*map)[k] = v;
	return map->size();
}

static inline int __kv_callback_vec(void* list, const string& k, const string& v) {
	auto *vec = (vector<query_kv_t>*)list;
	query_kv_t t ={k, v};
	vec->push_back(t);
	return vec->size();
}

size_t parse_query_string(unordered_map<string, string> *kvmap, const string& rawstr, bool strict) {
	return parseKeyValue(rawstr, __kv_callback_map, kvmap, strict);
}

size_t parse_query_string(vector<query_kv_t> *kvvec, const string& rawstr, bool strict) {
	return parseKeyValue(rawstr, __kv_callback_vec, kvvec, strict);
}

NAMESPACE_URI_END

#endif
