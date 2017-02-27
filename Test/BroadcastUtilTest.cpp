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
#include "RedisReplyFixtures.hpp"
#include "RedisConnection.hpp"
#include <string>
#include <ctime>

using namespace ::testing;

static const std::string TEST_APP_ID = "999";
static const std::string TEST_CHANNEL_NAME = "test:channel";
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

RedisResult getByDelay(RedisResult value) {
	std::cout << "waiting on blocking command...";
	sleep(1);
	std::cout << " done." << std::endl;
	return value;
}

TEST(BroadcastUtilTest, initializationSuccess) {
    // create a mock connection instance
	MockRedisConnection* conn = new MockRedisConnection(NULL, 0);

	// capture any call for blocking command
	EXPECT_CALL(*conn, cmd(StrEq("")))
		// and wait 1 sec and send empty response
		.WillRepeatedly(Return(getByDelay(RedisResult())));

	// setup mock to expect command
	std::string command = std::string("SUBSCRIBE ") + NAMESPACE_PREFIX + ":" + TEST_APP_ID + ":CHANNELS:CONTROL";
	EXPECT_CALL(*conn, isConnected())
		.Times(1)
		.WillOnce(Return(true));

	EXPECT_CALL(*conn, cmd(StartsWith(command.c_str())))
		// one time
		.Times(1)
		// and return back a blank reply
		.WillOnce(Return(RedisResult()));

	// verify that BroadcastUtil intializes
	ASSERT_TRUE(BroadcastUtil::initialize(TEST_APP_ID.c_str(), conn));
	ASSERT_TRUE(BroadcastUtil::isRunning());
	// sleep at least one second to make sure worker thread starts execution
	sleep(1);
	BroadcastUtil::stopAll();
}

TEST(BroadcastUtilTest, controlIncorrectChannel) {
    // create a mock connection instance
	MockRedisConnection* conn = new MockRedisConnection(NULL, 0);

	// capture any call for blocking command
	EXPECT_CALL(*conn, cmd(StrEq("")))
		// and wait 1 sec and send empty response
		.WillRepeatedly(Return(getByDelay(RedisResult())));

	// setup mock to expect command
	std::string command_1 = std::string("SUBSCRIBE ") + NAMESPACE_PREFIX + ":" + TEST_APP_ID + ":CHANNELS:CONTROL:";
	std::string command_2 = std::string("SUBSCRIBE ") + TEST_CHANNEL_NAME;

	EXPECT_CALL(*conn, isConnected())
	.Times(1)
	.WillOnce(Return(true));

	EXPECT_CALL(*conn, cmd(StartsWith(command_1.c_str())))
		// one time
		.Times(1)
		// and return back control command to subscribe but with incorrect channel
		.WillOnce(Return(getControlCommand(
				(std::string(NAMESPACE_PREFIX) + ":" + TEST_APP_ID + CONTROL + "fake_suffix").c_str(),
				(std::string("ADD_CHANNEL ") + TEST_CHANNEL_NAME).c_str())));

	EXPECT_CALL(*conn, cmd(StrEq(command_2.c_str())))
		// one time
		.Times(0);

	// initiate worker thread
	ASSERT_TRUE(BroadcastUtil::initialize(TEST_APP_ID.c_str(), conn));
	// sleep at least one second to make sure worker thread starts execution
	sleep(1);
	BroadcastUtil::stopAll();
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
