/*
 * RedisResultTest.cpp
 *
 *  Created on: Feb 22, 2017
 *      Author: bhadoria
 */

#include "RedisResult.hpp"
#include "hiredis/hiredis.h"
#include "gtest/gtest.h"

const char * TEST_STRING_VALUE = "test response";
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

TEST(RedisResultTest, DefaultValues) {
	RedisResult res = RedisResult();
	ASSERT_EQ(res.errMsg(), (const char *)NULL);
	ASSERT_EQ(res.strResult(), (const char *)NULL);
	ASSERT_EQ(res.resultType(), NONE);
	ASSERT_EQ(res.arrayResult(0).resultType(), NONE);
}

TEST(RedisResultTest, InitFromNull) {
	RedisResult res = RedisResult();
	res.setRedisReply(NULL);
	ASSERT_EQ(res.resultType(), ERROR);
}

TEST(RedisResultTest, InitFromError) {
	RedisResult res = RedisResult();
	redisReply* r = new redisReply();
	r->type = REDIS_REPLY_ERROR;
	res.setRedisReply(r);
	ASSERT_EQ(res.resultType(), ERROR);
}

TEST(RedisResultTest, NoMemLeak) {
	// need to move the whole test under DEATH test
	// because they are run as a child thread, and
	// pointer needs to be created and deleted within
	// the child thread to catch this condition
	ASSERT_DEATH({
		RedisResult res = RedisResult();
		redisReply* r = new redisReply();
		r->type = REDIS_REPLY_NIL;
		res.setRedisReply(r);
		// we are expecting below to cause
		// death, since r should already
		// have been freed up by our test class
		delete r;
	}, ".* pointer being freed was not allocated");
}

TEST(RedisResultTest, InitFromString) {
	RedisResult res = RedisResult();
	res.setRedisReply(getStringReply(TEST_STRING_VALUE));
	ASSERT_EQ(res.resultType(), STRING);
	ASSERT_STREQ(res.strResult(), TEST_STRING_VALUE);
	// make sure that its not a shallow pointer copy
	ASSERT_NE(res.strResult(), TEST_STRING_VALUE);
}

TEST(RedisResultTest, InitFromInteger) {
	RedisResult res = RedisResult();
	res.setRedisReply(getIntReply(TEST_INTEGER_VALUE));
	ASSERT_EQ(res.resultType(), INTEGER);
	ASSERT_EQ(res.intResult(), TEST_INTEGER_VALUE);
}

TEST(RedisResultTest, InitFromArray) {
	redisReply** values = new redisReply*[2];
	values[0] = getStringReply(TEST_STRING_VALUE);
	values[1] = getIntReply(TEST_INTEGER_VALUE);
	RedisResult res = RedisResult();
	res.setRedisReply(getArrayReply(2, values));
	ASSERT_EQ(res.resultType(), ARRAY);
	ASSERT_EQ(res.arrayResult(0).resultType(), STRING);
	ASSERT_EQ(res.arrayResult(1).resultType(), INTEGER);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
