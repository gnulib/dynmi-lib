/*
 * RedisConnectionTest.cpp
 *
 *  Created on: Feb 22, 2017
 *      Author: bhadoria
 */

#include "Dynmi/RedisResult.hpp"
#include "Dynmi/RedisConnection.hpp"
#include "gtest/gtest.h"

TEST(RedisConnectionTest, ReportsConnectionError) {
    // create a connection instance with invalid host
	RedisConnection conn("not valid", 1999);
    ASSERT_FALSE(conn.isConnected());
}

TEST(RedisConnectionTest, ReportsCommandFailure) {
    // create a connection instance with invalid host
    RedisConnection conn("not valid", 1999);
    RedisResult res = conn.cmd("keys *");
    ASSERT_EQ(res.resultType(), FAILED);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
