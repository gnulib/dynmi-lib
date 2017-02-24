/*
 * RedisConnectionTest.cpp
 *
 *  Created on: Feb 22, 2017
 *      Author: bhadoria
 */

#include "RedisResult.hpp"
#include "RedisConnection.hpp"
#include "gtest/gtest.h"

TEST(RedisConnectionTest, ReportsConnectionError) {
    // create a connection instance with invalid host
	RedisConnection conn("not valid", 1999);
    ASSERT_FALSE(conn.isConnected());
}

TEST(RedisConnectionTest, ReportsCommandError) {
    // create a connection instance with invalid host
    RedisConnection conn("not valid", 1999);
    RedisResult res = conn.cmd("keys *");
    ASSERT_EQ(res.resultType(), ERROR);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
