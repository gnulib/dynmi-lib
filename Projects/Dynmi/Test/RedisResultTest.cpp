/*
 * RedisResultTest.cpp
 *
 *  Created on: Feb 22, 2017
 *      Author: bhadoria
 */

#include "Dynmi/RedisResult.hpp"
#include "hiredis/hiredis.h"
#include "gtest/gtest.h"
#include "RedisReplyFixtures.hpp"

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
//	RedisResult res = RedisResult();
//	redisReply* r = new redisReply();
//	r->type = REDIS_REPLY_ERROR;
//	r->str = new char[strlen(TEST_STRING_ERR)];
//	r->len = strlen(TEST_STRING_ERR);
//	strcpy(r->str, TEST_STRING_ERR);
//	res.setRedisReply(r);
	RedisResult res = getErrorResult();
	ASSERT_EQ(res.resultType(), ERROR);
	ASSERT_STREQ(res.strResult(), NULL);
}

TEST(RedisResultTest, NoMemLeak) {
	// need to move the whole test under DEATH test
	// because they are run as a child thread, and
	// pointer needs to be created and deleted within
	// the child thread to catch this condition
	ASSERT_EXIT({
		RedisResult res = RedisResult();
		redisReply* r = new redisReply();
		r->type = REDIS_REPLY_NIL;
		res.setRedisReply(r);
		// we are expecting below to cause
		// death, since r should already
		// have been freed up by our test class
		delete r;
	}, ::testing::KilledBySignal(6), "");
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

TEST(RedisResultTest, ReuseAfterString) {
	RedisResult res = RedisResult();
	res.setRedisReply(getStringReply(TEST_STRING_VALUE));
	res.reuse();
	ASSERT_EQ(res.strResult(), (const char *)NULL);
	ASSERT_EQ(res.resultType(), NONE);
}

TEST(RedisResultTest, ReuseAfterInteger) {
	RedisResult res = RedisResult();
	res.setRedisReply(getIntReply(TEST_INTEGER_VALUE));
	res.reuse();
	ASSERT_NE(res.intResult(), TEST_INTEGER_VALUE);
	ASSERT_EQ(res.resultType(), NONE);
}

TEST(RedisResultTest, ReuseAfterError) {
//	RedisResult res = RedisResult();
//	redisReply* r = new redisReply();
//	r->type = REDIS_REPLY_ERROR;
//	r->str = new char[strlen(TEST_STRING_ERR)];
//	r->len = strlen(TEST_STRING_ERR);
//	strcpy(r->str, TEST_STRING_ERR);
//	res.setRedisReply(r);
	RedisResult res = getErrorResult();
	res.reuse();
	ASSERT_EQ(res.errMsg(), (const char *)NULL);
	ASSERT_EQ(res.resultType(), NONE);
}

TEST(RedisResultTest, ReuseAfterArray) {
	redisReply** values = new redisReply*[2];
	values[0] = getStringReply(TEST_STRING_VALUE);
	values[1] = getIntReply(TEST_INTEGER_VALUE);
	RedisResult res = RedisResult();
	res.setRedisReply(getArrayReply(2, values));
	res.reuse();
	ASSERT_EQ(res.resultType(), NONE);
	ASSERT_EQ(res.arraySize(), 0);
	ASSERT_EQ(res.arrayResult(0).resultType(), NONE);
}

TEST(RedisResultTest, InitFromArray) {
	redisReply** values = new redisReply*[2];
	values[0] = getStringReply(TEST_STRING_VALUE);
	values[1] = getIntReply(TEST_INTEGER_VALUE);
	RedisResult res = RedisResult();
	res.setRedisReply(getArrayReply(2, values));
	ASSERT_EQ(res.resultType(), ARRAY);
	ASSERT_EQ(res.arraySize(), 2);
	ASSERT_EQ(res.arrayResult(0).resultType(), STRING);
	ASSERT_EQ(res.arrayResult(1).resultType(), INTEGER);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
