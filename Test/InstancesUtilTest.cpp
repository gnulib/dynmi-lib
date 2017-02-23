/*
 * InstancesUtilTest.cpp
 *
 *  Created on: Feb 23, 2017
 *      Author: bhadoria
 */

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "InstancesUtil.hpp"
#include "RedisConnection.hpp"
#include "RedisResult.hpp"
#include "RedisReplyFixtures.hpp"

using namespace ::testing;

static const char * TEST_APP_ID = "999";

class MockRedisConnection : public RedisConnection {
public:
	MockRedisConnection(const char* host, int port) : RedisConnection(host, port){};
	MOCK_METHOD2(cmd, int(const char *, RedisResult&));
	MOCK_CONST_METHOD0(isConnected, bool());
};

TEST(InstancesUtilTest, ReserveInstanceIdNotConnected) {
    // create a mock connection instance
	MockRedisConnection conn(NULL, 0);

	// setup mock to expect isConnected called
	EXPECT_CALL(conn, isConnected())
		// one time
		.Times(1)
		// and return back false
		.WillOnce(Return(false));

	// verify that InstanceUtil returns back instance ID as -1
	// when redis connection is not valid
	ASSERT_EQ(InstancesUtil::reserveInstanceId(conn, TEST_APP_ID), -1);
}

TEST(InstancesUtilTest, ReserveInstanceIdErrorResponse) {
    // create a mock connection instance
	MockRedisConnection conn(NULL, 0);

	// setup mock to expect isConnected called
	EXPECT_CALL(conn, isConnected())
		// one time
		.Times(1)
		// and return back true
		.WillOnce(Return(true));

	// initialize redis response to be of type error
	RedisResult res = getErrorResult();

	// setup mock to expect command
	EXPECT_CALL(conn, cmd(_, _))
		// one time
		.Times(1)
		// and return back 0/success with response
		// as initialized above
		.WillOnce(Return(0));

	// verify that InstanceUtil returns back instance ID as -1
	// when redis response is of type error
	ASSERT_EQ(InstancesUtil::reserveInstanceId(conn, TEST_APP_ID), -1);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
