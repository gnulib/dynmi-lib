/*
 * RedisReplyFixtures.hpp
 *
 *  Created on: Feb 23, 2017
 *      Author: bhadoria
 */

#ifndef TEST_REDISREPLYFIXTURES_HPP_
#define TEST_REDISREPLYFIXTURES_HPP_

#include "hiredis/hiredis.h"

const char * TEST_STRING_VALUE = "test response";
const char * TEST_STRING_ERR = "this is error message";
const int TEST_INTEGER_VALUE = 102938475;

redisReply* getStringReply(const char * value) {
	redisReply* r = new redisReply;
	r->type = REDIS_REPLY_STRING;
	r->str = new char[strlen(value)];
	r->len = strlen(value);
	strcpy(r->str, value);
	return r;
}

redisReply* getIntReply(const int value) {
	redisReply* r = new redisReply;
	r->type = REDIS_REPLY_INTEGER;
	r->integer = value;
	return r;
}

redisReply* getArrayReply(int size, redisReply** values) {
	redisReply* r = new redisReply;
	r->type = REDIS_REPLY_ARRAY;
	r->elements = size;
	r->element = new redisReply*[size];
	for (int i=0; i< size; i++) {
		r->element[i] = values[i];
	}
	return r;
}

redisReply* getErrorReply() {
	redisReply* r = new redisReply();
	r->type = REDIS_REPLY_ERROR;
	r->str = new char[strlen(TEST_STRING_ERR)];
	r->len = strlen(TEST_STRING_ERR);
	strcpy(r->str, TEST_STRING_ERR);
	return r;
}

RedisResult getErrorResult() {
	RedisResult res = RedisResult();
	res.setRedisReply(getErrorReply());
	return res;
}

RedisResult getIntegerResult(const int value) {
	RedisResult res = RedisResult();
	res.setRedisReply(getIntReply(value));
	return res;
}

RedisResult getStringResult(const char* value) {
	RedisResult res = RedisResult();
	res.setRedisReply(getStringReply(value));
	return res;
}

RedisResult getArrayResult(int size, redisReply** values) {
	RedisResult res = RedisResult();
	res.setRedisReply(getArrayReply(size, values));
	return res;
}

#endif /* TEST_REDISREPLYFIXTURES_HPP_ */
