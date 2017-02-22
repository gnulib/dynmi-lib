/*
 * RedisResult.cpp
 *
 *  Created on: Feb 21, 2017
 *      Author: bhadoria
 */

#include "RedisResult.hpp"
#include "hiredis/hiredis.h"
#include <string.h>

RedisResult RedisResult::noResult = RedisResult();

RedisResult::RedisResult() {
	type = RedisResultType::NONE;
//	r = NULL;
	res = NULL;
	size = 0;
}

RedisResult::RedisResult(const RedisResult& other) {
//	type = other.type;
////	r = NULL;
//	res = NULL;
//	size = other.size;
//	if (other.type == RedisResultType::ARRAY) {
//		RedisResult* elements = new RedisResult[other.size];
//		for (int i=0; i< other.size; i++) {
//			elements[i] = ((RedisResult*)other.res)[i];
//		}
//		res = elements;
//	} else if (other.type == RedisResultType::STRING) {
//		res = new char[other.size];
//	}
	*this=other;
}

RedisResult& RedisResult::operator=(const RedisResult& other) {
	if (this != &other) {
		flush();
		type = other.type;
	//	r = NULL;
		res = NULL;
		size = other.size;
		if (other.type == RedisResultType::ARRAY) {
			RedisResult* elements = new RedisResult[other.size];
			for (int i=0; i< other.size; i++) {
				elements[i] = ((RedisResult*)other.res)[i];
			}
			res = elements;
		} else if (other.type == RedisResultType::STRING || other.type == RedisResultType::ERROR) {
			res = new char[other.size+1];
			strncpy((char *)res, (const char *)other.res, other.size);
		} else if (other.type == RedisResultType::INTEGER) {
			res = new int[1];
			*((int*)res) = *(int*)other.res;
		}
	}
	return *this;
}

RedisResult::~RedisResult() {
//	if (r) {
//	    freeReplyObject(r);
//	}
	flush();
}

void RedisResult::flush() {
//	if (res == NULL) return;
//	if (res != NULL) delete res;
	type = RedisResultType::NONE;
	size = 0;
	if (res != NULL) {
		if (type == RedisResultType::ARRAY) {
			//		for (; size > 0; ) {
			delete[] (RedisResult*) res;
			//		}
		} else if (type == RedisResultType::STRING || type == RedisResultType::ERROR) {
			delete[] (char *) res;
		} else if (type == RedisResultType::INTEGER) {
			delete[] (int *) res;
		}
	}
	res = NULL;
}

void RedisResult::reuse() {
//	if (r) {
//	    freeReplyObject(r);
//	}
//	r = NULL;
//	type = RedisResultType::NONE;
	flush();
}

const char* const RedisResult::strResult() const {
	if (type == RedisResultType::STRING) {
//		return r->str;
		return (const char *) res;
	} else {
		return NULL;
	}
}

int RedisResult::intResult() const {
	if (type == RedisResultType::INTEGER) {
//		return r->integer;
		return *(int*)res;
	} else {
		return 0;
	}
}

RedisResultType RedisResult::toMyType(int type) {
	RedisResultType myType = RedisResultType::NONE;
	switch(type) {
	case REDIS_REPLY_STRING:
		myType = RedisResultType::STRING;
		break;
	case REDIS_REPLY_INTEGER:
		myType = RedisResultType::INTEGER;
		break;
	case REDIS_REPLY_ARRAY:
		myType = RedisResultType::ARRAY;
		break;
	case REDIS_REPLY_ERROR:
		myType = RedisResultType::ERROR;
		break;
	default:
		break;
	}
	return myType;
}

int RedisResult::arraySize() const {
	if (type == RedisResultType::ARRAY) {
//		return r->elements;
		return size;
	} else {
		return 0;
	}
}

/**
 * how will the delete for redisResult work here???
 */
const RedisResult& RedisResult::arrayResult(int idx) const {
	RedisResult& item = RedisResult::noResult;
//	if (type == RedisResultType::ARRAY && idx <= r->elements) {
//		item.setRedisReply(r->element[idx]);
//	}
//	return item;
	if (type == RedisResultType::ARRAY && idx < size) {
		item = ((RedisResult*) res)[idx];
	}
	return item;
}

//RedisResultType RedisResult::arrayResultType(int idx) const {
//	RedisResultType idxType = RedisResultType::ERROR;
////	if (type == RedisResultType::ARRAY && idx <= r->elements) {
////		idxType = toMyType(r->element[idx]->type);
////	}
//	if (type == RedisResultType::ARRAY && idx <= size) {
//		idxType = ((RedisResult*)res)->type;
//	}
//	return idxType;
//}

//const char* const RedisResult::arrayStrResult(int idx) const {
//	if (arrayResultType(idx) == RedisResultType::STRING) {
//		return r->element[idx]->str;
//	}
//	return NULL;
//}

//int RedisResult::arrayIntResult(int idx) const {
//	if (arrayResultType(idx) == RedisResultType::INTEGER) {
//		return r->element[idx]->integer;
//	}
//	return 0;
//}

void RedisResult::fromRedisReply(redisReply* r) {
	void* res = NULL;
	type = RedisResult::toMyType(r->type);
	switch(type) {
	case RedisResultType::STRING:
		size = r->len;
		res = new char[size+1];
		strncpy((char *)res, r->str, size);
		((char*)res)[size] = NULL;
		break;
	case RedisResultType::INTEGER:
		size = 0;
		res = new int[1];
		*(int*)res = (int)(r->integer);
		break;
	case RedisResultType::ARRAY:
	{
		size = r->elements;
		RedisResult* elements = new RedisResult[size];
		for (int i=0; i < size; i++) {
			elements[i].setRedisReply(r->element[i], true);
		}
		res = elements;
	}
		break;
	case RedisResultType::ERROR:
		size = r->len;
		res = new char[size+1];
		strncpy((char *)res, r->str, size);
		((char*)res)[size] = NULL;
		break;
	default:
		break;
	}
	this->res = res;
}

const char* RedisResult::errMsg() const {
	if (type == RedisResultType::ERROR) {
		return (const char *) res;
	} else {
		return NULL;
	}
}
void RedisResult::setRedisReply(redisReply * r, bool internal) {
	if (r == NULL) {
		type = RedisResultType::ERROR;
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
