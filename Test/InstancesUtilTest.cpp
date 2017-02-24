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
#include <string>

using namespace ::testing;

static const char * TEST_APP_ID = "999";

class MockRedisConnection : public RedisConnection {
public:
	MockRedisConnection(const char* host, int port) : RedisConnection(host, port){};
	MOCK_METHOD1(cmd, RedisResult(const char *));
	MOCK_CONST_METHOD0(isConnected, bool());
};

TEST(InstancesUtilTest, ReserveInstanceIdNonIntegerResultType) {
    // create a mock connection instance
	MockRedisConnection conn(NULL, 0);

	// setup mock to expect command
	EXPECT_CALL(conn, cmd(_))
		// one time
		.Times(1)
		// and return back 0/success as return code
		// and operation result with default/Null type
		.WillOnce(Return(RedisResult()));

	// verify that InstanceUtil returns back instance ID as -1
	// when redis operation result is of type error
	ASSERT_EQ(InstancesUtil::reserveInstanceId(conn, TEST_APP_ID), -1);
}

TEST(InstancesUtilTest, ReserveInstanceIdCommandFailure) {
    // create a mock connection instance
	MockRedisConnection conn(NULL, 0);

	// setup mock to expect command
	EXPECT_CALL(conn, cmd(_))
		// one time
		.Times(1)
		// and return back error as return type
		// to simulate command execution failure
		.WillOnce(Return(getErrorResult()));

	// verify that InstanceUtil returns back instance ID as -1
	// when command execution fails
	ASSERT_EQ(InstancesUtil::reserveInstanceId(conn, TEST_APP_ID), -1);
}

TEST(InstancesUtilTest, ReserveInstanceIdCommandSchema) {
    // create a mock connection instance
	MockRedisConnection conn(NULL, 0);

	// setup mock to expect command with following schema
	// INCR <NAMESPACE PREFIX>:<App ID>:CURR_INSTANCE
	std::string command = std::string("INCR ") + INSTANCES_UTIL_NAMESPACE + ":" + TEST_APP_ID + ":CURR_INSTANCE";
	EXPECT_CALL(conn, cmd(StrEq(command.c_str())))
		// one time
		.Times(1)
		// and return back an integer reply
		.WillOnce(Return(getIntegerResult(1)));

	// invoke the utility with request to reserver instance ID
	InstancesUtil::reserveInstanceId(conn, TEST_APP_ID);
}

TEST(InstancesUtilTest, ReserveInstanceIdSuccess) {
    // create a mock connection instance
	MockRedisConnection conn(NULL, 0);

	// initialize redis operation result as INTEGER type
	RedisResult res = RedisResult();
	res.setRedisReply(getIntReply(1));

	// setup mock to expect command
	EXPECT_CALL(conn, cmd(_))
		// one time
		.Times(1)
		// and return back an integer reply
		.WillOnce(Return(getIntegerResult(99)));

	// verify that InstanceUtil returns back instance ID same as
	// what we initialized our redis response above
	ASSERT_EQ(InstancesUtil::reserveInstanceId(conn, TEST_APP_ID), 99);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
