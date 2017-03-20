/*
 * RedisResult.cpp
 *
 *  Created on: Feb 21, 2017
 *      Author: bhadoria
 */

#include "Dynmi/RedisResult.hpp"
#include "hiredis/hiredis.h"
#include <string.h>
#include <cstddef>

RedisResult RedisResult::noResult = RedisResult();

RedisResult::RedisResult() {
	type = NONE;
	res = NULL;
	size = 0;
}

RedisResult::RedisResult(const RedisResult& other) {
	*this=other;
}

RedisResult& RedisResult::operator=(const RedisResult& other) {
	if (this != &other) {
		flush();
		type = other.type;
		res = NULL;
		size = other.size;
		if (other.type == ARRAY) {
			RedisResult* elements = new RedisResult[other.size];
			for (int i=0; i< other.size; i++) {
				elements[i] = ((RedisResult*)other.res)[i];
			}
			res = elements;
		} else if (other.type == STRING || other.type == ERROR) {
			res = new char[other.size+1];
			strncpy((char *)res, (const char *)other.res, other.size);
			((char*)res)[size] = 0;
		} else if (other.type == INTEGER) {
			res = new int[1];
			*((int*)res) = *(int*)other.res;
		}
	}
	return *this;
}

RedisResult::~RedisResult() {
	flush();
}

void RedisResult::flush() {
	type = NONE;
	size = 0;
	if (res != NULL) {
		if (type == ARRAY) {
			//		for (; size > 0; ) {
			delete[] (RedisResult*) res;
			//		}
		} else if (type == STRING || type == ERROR) {
			delete[] (char *) res;
		} else if (type == INTEGER) {
			delete[] (int *) res;
		}
	}
	res = NULL;
}

void RedisResult::reuse() {
	flush();
}

const char* const RedisResult::strResult() const {
	if (type == STRING) {
		return (const char *) res;
	} else {
		return NULL;
	}
}

int RedisResult::intResult() const {
	if (type == INTEGER) {
		return *(int*)res;
	} else {
		return 0;
	}
}

RedisResultType RedisResult::toMyType(int type) {
	RedisResultType myType = NONE;
	switch(type) {
	case REDIS_REPLY_STRING:
		myType = STRING;
		break;
	case REDIS_REPLY_INTEGER:
		myType = INTEGER;
		break;
	case REDIS_REPLY_ARRAY:
		myType = ARRAY;
		break;
	case REDIS_REPLY_ERROR:
		myType = ERROR;
		break;
	default:
		break;
	}
	return myType;
}

int RedisResult::arraySize() const {
	if (type == ARRAY) {
		return size;
	} else {
		return 0;
	}
}

const RedisResult& RedisResult::arrayResult(int idx) const {
	RedisResult& item = RedisResult::noResult;
	if (type == ARRAY && idx < size) {
		item = ((RedisResult*) res)[idx];
	}
	return item;
}


void RedisResult::fromRedisReply(redisReply* r) {
	void* res = NULL;
	type = RedisResult::toMyType(r->type);
	switch(type) {
	case STRING:
		size = r->len;
		res = new char[size+1];
		strncpy((char *)res, r->str, size);
		((char*)res)[size] = 0;
		break;
	case INTEGER:
		size = 0;
		res = new int[1];
		*(int*)res = (int)(r->integer);
		break;
	case ARRAY:
	{
		size = r->elements;
		RedisResult* elements = new RedisResult[size];
		for (int i=0; i < size; i++) {
			elements[i].setRedisReply(r->element[i], true);
		}
		res = elements;
	}
		break;
	case ERROR:
		size = r->len;
		res = new char[size+1];
		strncpy((char *)res, r->str, size);
		((char*)res)[size] = 0;
		break;
	default:
		break;
	}
	this->res = res;
}

const char* RedisResult::errMsg() const {
	if (type == ERROR) {
		return (const char *) res;
	} else {
		return NULL;
	}
}
void RedisResult::setRedisReply(redisReply * r, bool internal) {
	if (r == NULL) {
		type = ERROR;
	} else {
		fromRedisReply(r);
	}
	if (!internal) {
	    // need to free the redis reply explicitly
	    freeReplyObject(r);
	}
}

void RedisResult::setRedisReply(redisReply * r) {
	reuse();
	setRedisReply(r, false);
}
