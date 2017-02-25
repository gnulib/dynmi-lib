/*
 * BroadcastUtilTest.cpp
 *
 *  Created on: Feb 24, 2017
 *      Author: bhadoria
 */

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "BroadcastUtil.hpp"
#include "RedisResult.hpp"
#include "RedisConnection.hpp"
#include <string>
#include <ctime>

using namespace ::testing;

static const std::string TEST_APP_ID = "999";
static const std::string TEST_NODE_ID = "99";
static const std::string TEST_HOST = "test.host";
static const std::string TEST_PORT = "2938";
static const std::string TEST_LOCK_NAME = "test-lock";

class MockRedisConnection : public RedisConnection {
public:
	MockRedisConnection(const char* host, int port) : RedisConnection(host, port){};
	MOCK_METHOD1(cmd, RedisResult(const char *));
	MOCK_CONST_METHOD0(isConnected, bool());
};

TEST(BroadcastUtilTest, initializationSuccess) {
    // create a mock connection instance
	MockRedisConnection* conn = new MockRedisConnection(NULL, 0);

	// setup mock to expect command
	std::string command = std::string("SUBSCRIBE ") + NAMESPACE_PREFIX + ":" + TEST_APP_ID + ":CHANNELS:CONTROL";
	EXPECT_CALL(*conn, isConnected())
	.Times(1)
	.WillOnce(Return(true));

	EXPECT_CALL(*conn, cmd(StartsWith(command.c_str())))
		// one time
		.Times(1)
		// and return back an integer reply
		.WillOnce(Return(RedisResult()));

	// verify that BroadcastUtil intializes
	std::cout << "Initialization started" << std::endl;
	ASSERT_TRUE(BroadcastUtil::initialize(TEST_APP_ID.c_str(), conn));
	std::cout << "Initialization complete" << std::endl;
	BroadcastUtil::stopAll();
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
