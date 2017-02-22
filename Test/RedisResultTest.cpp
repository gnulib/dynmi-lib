/*
 * RedisResultTest.cpp
 *
 *  Created on: Feb 22, 2017
 *      Author: bhadoria
 */

#include "RedisResult.hpp"
#include "gtest/gtest.h"

TEST(RedisResultTest, DefaultValues) {
	RedisResult res = RedisResult();
	ASSERT_EQ(res.errMsg(), (const char *)NULL);
	ASSERT_EQ(res.strResult(), (const char *)NULL);
	ASSERT_EQ(res.resultType(), NONE);
	ASSERT_EQ(res.arrayResult(0).resultType(), NONE);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
