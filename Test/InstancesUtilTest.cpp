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

static const std::string TEST_APP_ID = "999";
static const std::string TEST_NODE_ID = "99";
static const std::string TEST_HOST = "test.host";
static const std::string TEST_PORT = "2938";

class MockRedisConnection : public RedisConnection {
public:
	MockRedisConnection(const char* host, int port) : RedisConnection(host, port){};
	MOCK_METHOD1(cmd, RedisResult(const char *));
	MOCK_CONST_METHOD0(isConnected, bool());
};

TEST(InstancesUtilTest, getNewInstanceIdNonIntegerResultType) {
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
	ASSERT_EQ(InstancesUtil::getNewInstanceId(conn, TEST_APP_ID.c_str()), -1);
}

TEST(InstancesUtilTest, getNewInstanceIdCommandFailure) {
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
	ASSERT_EQ(InstancesUtil::getNewInstanceId(conn, TEST_APP_ID.c_str()), -1);
}

TEST(InstancesUtilTest, getNewInstanceIdKeySchema) {
    // create a mock connection instance
	MockRedisConnection conn(NULL, 0);

	// setup mock to expect command with following schema
	// INCR <NAMESPACE PREFIX>:<App ID>:NODES
	std::string command = std::string("INCR ") + INSTANCES_UTIL_NAMESPACE + ":" + TEST_APP_ID + ":NODES";
	EXPECT_CALL(conn, cmd(StrEq(command.c_str())))
		// one time
		.Times(1)
		// and return back an integer reply
		.WillOnce(Return(getIntegerResult(1)));

	// invoke the utility with request to reserver instance ID
	InstancesUtil::getNewInstanceId(conn, TEST_APP_ID.c_str());
}

TEST(InstancesUtilTest, getNewInstanceIdSuccess) {
    // create a mock connection instance
	MockRedisConnection conn(NULL, 0);

	// setup mock to expect command
	EXPECT_CALL(conn, cmd(_))
		// one time
		.Times(1)
		// and return back an integer reply
		.WillOnce(Return(getIntegerResult(std::atoi(TEST_NODE_ID.c_str()))));

	// verify that InstanceUtil returns back instance ID same as
	// what we initialized our redis response above
	ASSERT_EQ(InstancesUtil::getNewInstanceId(conn, TEST_APP_ID.c_str()), 99);
}

TEST(InstancesUtilTest, publishNodeDetailsCommandSequence) {
    // create a mock connection instance
	MockRedisConnection conn(NULL, 0);
	// <NAMESPACE PREFIX>:<App ID>:INSTANCE:<Node ID>:ADDRESS (TTL set to specified value)
	std::string command1 = std::string("HMSET ") + INSTANCES_UTIL_NAMESPACE
							+ ":" + TEST_APP_ID + ":INSTANCE:" + TEST_NODE_ID
							+ ":ADDRESS HOST " + TEST_HOST + " PORT " + TEST_PORT;
	std::string command2 = std::string("EXPIRE ") + INSTANCES_UTIL_NAMESPACE
							+ ":" + TEST_APP_ID + ":INSTANCE:" + TEST_NODE_ID
							+ ":ADDRESS 20";
	std::string command3 = std::string("SADD ") + INSTANCES_UTIL_NAMESPACE
			+ ":" + TEST_APP_ID + ":INSTANCES " + TEST_NODE_ID;

	std::string command4 = std::string("PUBLISH ") + INSTANCES_UTIL_NAMESPACE
			+ ":" + TEST_APP_ID + ":CHANNELS:INSTANCES " + TEST_NODE_ID;
	// expect following calls in sequence
	{
		InSequence dummy;
		EXPECT_CALL(conn, isConnected())
		.Times(1)
		.WillOnce(Return(true));
		EXPECT_CALL(conn, cmd(StrEq(command1.c_str())))
		.Times(1)
		.WillOnce(Return(RedisResult()));
		EXPECT_CALL(conn, cmd(StrEq(command2.c_str())))
		.Times(1)
		.WillOnce(Return(RedisResult()));
		EXPECT_CALL(conn, cmd(StrEq(command3.c_str())))
		.Times(1)
		.WillOnce(Return(getIntegerResult(1)));
		EXPECT_CALL(conn, cmd(StrEq(command4.c_str())))
		.Times(1)
		.WillOnce(Return(getIntegerResult(1)));
	}
	// call the utility method to publish node's address
	// and expect 1 node as subscriber
	ASSERT_EQ(InstancesUtil::publishNodeDetails(conn, TEST_APP_ID.c_str(), std::atoi(TEST_NODE_ID.c_str()), TEST_HOST.c_str(), std::atoi(TEST_PORT.c_str()), 20), 1);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
