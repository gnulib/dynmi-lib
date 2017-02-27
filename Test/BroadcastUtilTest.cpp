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
static const std::string TEST_NODE_ID = "1";
static const std::string TEST_CHANNEL_NAME = "test:channel";
static const std::string TEST_BROADCAST = "\"this is a test\"";

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

void test_call_back(const char* msg) {
	std::cout << "##### Received message: " << msg << std::endl;
}

TEST(BroadcastUtilTest, initializationSuccess) {
    // create a mock connection instance for worker thread
	MockRedisConnection* workerConn = new MockRedisConnection(NULL, 0);
    // create a mock connection instance for test case use
	MockRedisConnection myConn(NULL, 0);


	// capture any call for blocking command
	EXPECT_CALL(*workerConn, cmd(StrEq("")))
		// and wait 1 sec and send empty response
		.WillRepeatedly(Return(getByDelay(RedisResult())));

	// setup mock to expect command
	std::string command_1 = std::string("SUBSCRIBE ") + NAMESPACE_PREFIX + ":" + TEST_APP_ID + ":CHANNELS:CONTROL:"+ TEST_NODE_ID;
	//	std::string stop_command = std::string("PUBLISH ") + NAMESPACE_PREFIX + ":" + TEST_APP_ID + ":CHANNELS:CONTROL:.+ STOP";
	std::string stop_command = std::string("PUBLISH ") + NAMESPACE_PREFIX + ":" + TEST_APP_ID + ":CHANNELS:CONTROL:" + TEST_NODE_ID +" STOP";
	EXPECT_CALL(*workerConn, isConnected())
		.Times(1)
		.WillOnce(Return(true));

	EXPECT_CALL(*workerConn, cmd(StrEq(command_1.c_str())))
		// one time
		.Times(1)
		// and return back a blank reply
		.WillOnce(Return(RedisResult()));

	EXPECT_CALL(myConn, cmd(MatchesRegex(stop_command.c_str())))
		// one time
		.Times(1)
		// and return back an integer reply
		.WillOnce(Return(getIntegerResult(1)));

	// verify that BroadcastUtil intializes
	ASSERT_TRUE(BroadcastUtil::initialize(TEST_APP_ID.c_str(), TEST_NODE_ID.c_str(), workerConn));
	ASSERT_TRUE(BroadcastUtil::isRunning());
	// sleep at least one second to make sure worker thread starts execution
	sleep(1);
	BroadcastUtil::stopAll(myConn);
}

TEST(BroadcastUtilTest, controlIncorrectChannel) {
    // create a mock connection instance for worker thread
	MockRedisConnection* conn = new MockRedisConnection(NULL, 0);
    // create a mock connection instance for test case use
	MockRedisConnection myConn(NULL, 0);

	// capture any call for blocking command
	EXPECT_CALL(*conn, cmd(StrEq("")))
		// and wait 1 sec and send empty response
		.WillRepeatedly(Return(getByDelay(RedisResult())));

	// setup mock to expect command
	std::string command_1 = std::string("SUBSCRIBE ") + NAMESPACE_PREFIX + ":" + TEST_APP_ID + ":CHANNELS:CONTROL:"+ TEST_NODE_ID;
	std::string command_2 = std::string("SUBSCRIBE ") + TEST_CHANNEL_NAME;
	std::string stop_command = std::string("PUBLISH ") + NAMESPACE_PREFIX + ":" + TEST_APP_ID + ":CHANNELS:CONTROL:" + TEST_NODE_ID +" STOP";

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

	EXPECT_CALL(myConn, cmd(MatchesRegex(stop_command.c_str())))
		// one time
		.Times(1)
		// and return back an integer reply
		.WillOnce(Return(getIntegerResult(1)));

	// initiate worker thread
	ASSERT_TRUE(BroadcastUtil::initialize(TEST_APP_ID.c_str(), TEST_NODE_ID.c_str(), conn));
	// sleep at least one second to make sure worker thread starts execution
	sleep(1);
	BroadcastUtil::stopAll(myConn);
}

//TEST(BroadcastUtilTest, addSubscription) {
//    // create a mock connection instance for worker thread
//	MockRedisConnection* conn = new MockRedisConnection(NULL, 0);
//    // create a mock connection instance for test case use
//	MockRedisConnection myConn(NULL, 0);
//
//	// capture any call for blocking command
//	EXPECT_CALL(*conn, cmd(StrEq("")))
//		// and wait 1 sec and send empty response
//		.WillRepeatedly(Return(getByDelay(RedisResult())));
//
//	// setup mock to expect command
//	std::string command_1 = std::string("SUBSCRIBE ") + NAMESPACE_PREFIX + ":" + TEST_APP_ID + ":CHANNELS:CONTROL:"+ TEST_NODE_ID;
//	std::string command_2 = std::string("PUBLISH ") + NAMESPACE_PREFIX + ":" + TEST_APP_ID + ":CHANNELS:CONTROL:.+ \"ADD_CHANNEL " + TEST_CHANNEL_NAME + "\"";
//	std::string command_3 = std::string("PUBLISH ") + TEST_CHANNEL_NAME + " " + TEST_BROADCAST;
//	std::string stop_command = std::string("PUBLISH ") + NAMESPACE_PREFIX + ":" + TEST_APP_ID + ":CHANNELS:CONTROL:" + TEST_NODE_ID +" STOP";
//
//	EXPECT_CALL(*conn, isConnected())
//	.Times(1)
//	.WillOnce(Return(true));
//
//	EXPECT_CALL(*conn, cmd(StrEq(command_1.c_str())))
//		// one time
//		.Times(1)
//		// and return back control command to subscribe
//		.WillOnce(Return(getControlCommand(
//				(std::string(NAMESPACE_PREFIX) + ":" + TEST_APP_ID + CONTROL + TEST_NODE_ID).c_str(),
//				(std::string("ADD_CHANNEL ") + TEST_CHANNEL_NAME).c_str())));
//
//	EXPECT_CALL(myConn, cmd(MatchesRegex(command_2.c_str())))
//		// one time
//		.Times(1)
//		// and return back an integer reply
//		.WillOnce(Return(getIntegerResult(1)));
//
//	EXPECT_CALL(myConn, cmd(MatchesRegex(command_3.c_str())))
//		// one time
//		.Times(1)
//		// and return back an integer reply
//		.WillOnce(Return(getIntegerResult(1)));
//
//	EXPECT_CALL(myConn, cmd(MatchesRegex(stop_command.c_str())))
//		// one time
//		.Times(1)
//		// and return back an integer reply
//		.WillOnce(Return(getIntegerResult(1)));
//
//	// initiate worker thread
//	ASSERT_TRUE(BroadcastUtil::initialize(TEST_APP_ID.c_str(), TEST_NODE_ID.c_str(), conn));
//	// subscribe to test channel
//	ASSERT_EQ(BroadcastUtil::addSubscription(myConn, TEST_CHANNEL_NAME.c_str(), test_call_back), 1);
//	// send a test message on our test channel
//	BroadcastUtil::publish(myConn, TEST_CHANNEL_NAME.c_str(), TEST_BROADCAST.c_str());
//
//	// sleep at least one second to make sure worker thread starts execution
//	sleep(1);
//	BroadcastUtil::stopAll(myConn);
//}

TEST(BroadcastUtilTest, controlAddSubscription) {
    // create a mock connection instance for worker thread
	MockRedisConnection* conn = new MockRedisConnection(NULL, 0);
    // create a mock connection instance for test case use
	MockRedisConnection myConn(NULL, 0);

	// capture any call for blocking command
	EXPECT_CALL(*conn, cmd(StrEq("")))
		// and wait 1 sec and send empty response
	.WillRepeatedly(Return(getByDelay(RedisResult())));

	// setup mock to expect command
	std::string command_1 = std::string("SUBSCRIBE ") + NAMESPACE_PREFIX + ":" + TEST_APP_ID + ":CHANNELS:CONTROL:"+ TEST_NODE_ID;
	std::string command_2 = std::string("PUBLISH ") + NAMESPACE_PREFIX + ":" + TEST_APP_ID + ":CHANNELS:CONTROL:.+ \"ADD_CHANNEL " + TEST_CHANNEL_NAME + "\"";
	std::string command_3 = std::string("SUBSCRIBE ") + TEST_CHANNEL_NAME;
	std::string command_4 = std::string("PUBLISH ") + TEST_CHANNEL_NAME + " " + TEST_BROADCAST;
	std::string stop_command = std::string("PUBLISH ") + NAMESPACE_PREFIX + ":" + TEST_APP_ID + ":CHANNELS:CONTROL:" + TEST_NODE_ID +" STOP";

	EXPECT_CALL(*conn, isConnected())
	.Times(1)
	.WillOnce(Return(true));

	EXPECT_CALL(*conn, cmd(StrEq(command_1.c_str())))
		// one time
		.Times(1)
		// and return back control command to subscribe
		.WillOnce(Return(getControlCommand(
				(std::string(NAMESPACE_PREFIX) + ":" + TEST_APP_ID + CONTROL + TEST_NODE_ID).c_str(),
				(std::string("ADD_CHANNEL ") + TEST_CHANNEL_NAME).c_str())));

	EXPECT_CALL(myConn, cmd(MatchesRegex(command_2.c_str())))
		// one time
		.Times(1)
		// and return back an integer reply
		.WillOnce(Return(getIntegerResult(1)));

	// capture any call for blocking command
	EXPECT_CALL(*conn, cmd(StrEq(command_3.c_str())))
		// one time
		.Times(1)
		// and wait 1 sec and send empty response
		.WillOnce(Return(getByDelay(getControlCommand(
				(TEST_CHANNEL_NAME).c_str(),
				(TEST_BROADCAST).c_str()))));

	EXPECT_CALL(myConn, cmd(MatchesRegex(command_4.c_str())))
		// one time
		.Times(1)
		// and return back an integer reply
		.WillOnce(Return(getIntegerResult(1)));

	EXPECT_CALL(myConn, cmd(MatchesRegex(stop_command.c_str())))
		// one time
		.Times(1)
		// and return back an integer reply
		.WillOnce(Return(getIntegerResult(1)));

	// initiate worker thread
	ASSERT_TRUE(BroadcastUtil::initialize(TEST_APP_ID.c_str(), TEST_NODE_ID.c_str(), conn));
	// subscribe to test channel
	ASSERT_EQ(BroadcastUtil::addSubscription(myConn, TEST_CHANNEL_NAME.c_str(), test_call_back), 1);
	// send a test message on our test channel
	BroadcastUtil::publish(myConn, TEST_CHANNEL_NAME.c_str(), TEST_BROADCAST.c_str());

	// sleep at least one second to make sure worker thread starts execution
	sleep(1);
	BroadcastUtil::stopAll(myConn);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
