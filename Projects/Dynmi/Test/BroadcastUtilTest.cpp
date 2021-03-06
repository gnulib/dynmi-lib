/*
 * BroadcastUtilTest.cpp
 *
 *  Created on: Feb 24, 2017
 *      Author: bhadoria
 */

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "Dynmi/BroadcastUtil.hpp"
#include "Dynmi/RedisResult.hpp"
#include "Dynmi/RedisConnection.hpp"
#include "Dynmi/RedisConnectionTL.hpp"
#include "Dynmi/DynmiGlobals.hpp"
#include "RedisReplyFixtures.hpp"
#include "MockRedisConnection.hpp"
#include <string>
#include <ctime>

using namespace ::testing;

static const std::string TEST_APP_ID = "999";
static const std::string TEST_NODE_ID = "1";
static const std::string TEST_CHANNEL_NAME = "test:channel";
static const std::string TEST_BROADCAST = "\"this is a test\"";

RedisResult getByDelay(RedisResult value) {
	std::cout << "waiting on blocking command...";
	sleep(1);
	std::cout << " done." << std::endl;
	return value;
}

void test_call_back(const char* channel, const char* msg) {
	std::cout << "##### Received message: " << msg << std::endl;
}

TEST(BroadcastUtilTest, initializationSuccess) {
    // create a mock connection instance for worker thread
	MockRedisConnection* conn = new MockRedisConnection(NULL, 0);
	RedisConnectionTL::initializeTest(conn);

	// handle worker thread clone
	EXPECT_CALL(*conn, clone())
	.WillRepeatedly(Return(*((RedisConnection*)conn)));

	// capture any call for blocking command
	EXPECT_CALL(*conn, cmd(StrEq("")))
		// and wait 1 sec and send empty response
		.WillRepeatedly(Return(getByDelay(RedisResult())));

	// setup mock to expect command
	std::string command_1 = std::string("SUBSCRIBE ") + NAMESPACE_PREFIX + ":" + TEST_APP_ID + ":CHANNELS:CONTROL:"+ TEST_NODE_ID;
	std::string control_channel = std::string(NAMESPACE_PREFIX) + ":" + TEST_APP_ID + ":CHANNELS:CONTROL:" + TEST_NODE_ID;
	EXPECT_CALL(*conn, isConnected())
	.WillRepeatedly(Return(true));

	EXPECT_CALL(*conn, cmd(StrEq(command_1.c_str())))
		// one time
		.Times(1)
		// and return back a blank reply
		.WillOnce(Return(RedisResult()));

	EXPECT_CALL(*conn, publish(StrEq(control_channel.c_str()), StrEq("STOP")))
		// one time
		.Times(1)
		// and return back an integer reply
		.WillOnce(Return(getIntegerResult(1)));

	// verify that BroadcastUtil intializes
	ASSERT_TRUE(BroadcastUtil::initialize(TEST_APP_ID.c_str(), TEST_NODE_ID.c_str()));
	ASSERT_TRUE(BroadcastUtil::instance().isRunning());
	// sleep at least one second to make sure worker thread starts execution
	sleep(1);
	BroadcastUtil::stopAll();
	delete conn;
}

/**
 * test worker thread logic to handle incorrect control channel messages
 */
TEST(BroadcastUtilTest, controlIncorrectChannel) {
    // create a mock connection instance for worker thread
	MockRedisConnection* conn = new MockRedisConnection(NULL, 0);
	RedisConnectionTL::initializeTest(conn);

	// handle worker thread clone
	EXPECT_CALL(*conn, clone())
	.WillRepeatedly(Return(*((RedisConnection*)conn)));

	// capture any call for blocking command
	EXPECT_CALL(*conn, cmd(StrEq("")))
		// and wait 1 sec and send empty response
		.WillRepeatedly(Return(getByDelay(RedisResult())));

	// setup mock to expect command
	std::string control_channel = std::string(NAMESPACE_PREFIX) + ":" + TEST_APP_ID + ":CHANNELS:CONTROL:" + TEST_NODE_ID;
	std::string command_1 = std::string("SUBSCRIBE ") + control_channel;
	std::string command_2 = std::string("SUBSCRIBE ") + TEST_CHANNEL_NAME;

	EXPECT_CALL(*conn, isConnected())
	.WillRepeatedly(Return(true));

	EXPECT_CALL(*conn, cmd(StartsWith(command_1.c_str())))
		// one time
		.Times(1)
		// and return back control command to subscribe but with incorrect channel
		.WillOnce(Return(getControlCommand(
				(std::string(NAMESPACE_PREFIX) + ":" + TEST_APP_ID + CONTROL + "fake_suffix").c_str(),
				(std::string("ADD_CHANNEL ") + TEST_CHANNEL_NAME).c_str())));

	EXPECT_CALL(*conn, cmd(StrEq(command_2.c_str())))
		// should never be called
		.Times(0);

	EXPECT_CALL(*conn, publish(StrEq(control_channel.c_str()), StrEq("STOP")))
		// one time
		.Times(1)
		// and return back an integer reply
		.WillOnce(Return(getIntegerResult(1)));

	// initiate worker thread
	ASSERT_TRUE(BroadcastUtil::initialize(TEST_APP_ID.c_str(), TEST_NODE_ID.c_str()));
	// sleep at least one second to make sure worker thread starts execution
	sleep(1);
	BroadcastUtil::stopAll();
	delete conn;
}

/**
 * test worker thread logic to handle data messages on a subscribed channel
 */
TEST(BroadcastUtilTest, controlDataMessage) {
    // create a mock connection instance for worker thread
	MockRedisConnection* conn = new MockRedisConnection(NULL, 0);
	RedisConnectionTL::initializeTest(conn);

	// setup mock to expect command
	std::string control_channel = std::string(NAMESPACE_PREFIX) + ":" + TEST_APP_ID + ":CHANNELS:CONTROL:" + TEST_NODE_ID;
	std::string command_1 = std::string("SUBSCRIBE ") + control_channel;
	std::string command_2 = "ADD_CHANNEL " + TEST_CHANNEL_NAME;
	std::string command_3 = std::string("SUBSCRIBE ") + TEST_CHANNEL_NAME;

	// handle worker thread clone
	EXPECT_CALL(*conn, clone())
	.WillRepeatedly(Return(*((RedisConnection*)conn)));

	// return connected when asked
	EXPECT_CALL(*conn, isConnected())
	.WillRepeatedly(Return(true));

	// return blank result every 1 second when waiting for message
	EXPECT_CALL(*conn, cmd(StrEq("")))
	.WillRepeatedly(Return(getByDelay(RedisResult())));

	// return a subscription control command after control channel has been subscribed
	EXPECT_CALL(*conn, cmd(StrEq(command_1.c_str())))
		.Times(1)
		.WillOnce(Return(getControlCommand(
				(std::string(NAMESPACE_PREFIX) + ":" + TEST_APP_ID + CONTROL + TEST_NODE_ID).c_str(),
				(command_2).c_str())));

	// return a data message on channel after the channel has been subscribed to
	EXPECT_CALL(*conn, cmd(StrEq(command_3.c_str())))
		.Times(1)
		.WillOnce(Return(getByDelay(getControlCommand(
				(TEST_CHANNEL_NAME).c_str(),
				(TEST_BROADCAST).c_str()))));

	// mock up handling of control command sent to subscribe to a channel
	EXPECT_CALL(*conn, publish(StrEq(control_channel.c_str()), StrEq(command_2.c_str())))
		// one time
		.Times(1)
		// and return back an integer reply
		.WillOnce(Return(getIntegerResult(1)));

	// mock to handle stop request
	EXPECT_CALL(*conn, publish(StrEq(control_channel.c_str()), StrEq("STOP")))
		.Times(1)
		.WillOnce(Return(getIntegerResult(1)));

	// initiate worker thread
	ASSERT_TRUE(BroadcastUtil::initialize(TEST_APP_ID.c_str(), TEST_NODE_ID.c_str()));
	// subscribe to test channel
	ASSERT_EQ(BroadcastUtil::instance().addSubscription(TEST_CHANNEL_NAME.c_str(), test_call_back), 1);

	// sleep at least one second to make sure worker thread starts execution
	sleep(1);
	BroadcastUtil::stopAll();
	delete conn;
}

/**
 * test API method to publish message to a channel
 */
TEST(BroadcastUtilTest, commandPublishMessage) {
    // create a mock connection instance for worker thread
	MockRedisConnection* conn = new MockRedisConnection(NULL, 0);
	RedisConnectionTL::initializeTest(conn);

	// setup mock to expect command
	std::string control_channel = std::string(NAMESPACE_PREFIX) + ":" + TEST_APP_ID + ":CHANNELS:CONTROL:" + TEST_NODE_ID;
	std::string command_1 = std::string("SUBSCRIBE ") + control_channel;
	std::string command_2 = "ADD_CHANNEL " + TEST_CHANNEL_NAME;

	// handle worker thread clone
	EXPECT_CALL(*conn, clone())
	.WillRepeatedly(Return(*((RedisConnection*)conn)));

	// return connected when asked
	EXPECT_CALL(*conn, isConnected())
	.WillRepeatedly(Return(true));

	// return blank result every 1 second when waiting for message
	EXPECT_CALL(*conn, cmd(StrEq("")))
	.WillRepeatedly(Return(getByDelay(RedisResult())));

	// return a blank result after control channel has been subscribed
	EXPECT_CALL(*conn, cmd(StrEq(command_1.c_str())))
		// one time
		.Times(1)
		// and return back control command to subscribe
		.WillOnce(Return(getByDelay(RedisResult())));

	// verify that correct command issued to publish message to channel
	EXPECT_CALL(*conn, publish(StrEq(TEST_CHANNEL_NAME.c_str()), StrEq(TEST_BROADCAST.c_str())))
		// one time
		.Times(1)
		// and return back an integer reply
		.WillOnce(Return(getIntegerResult(1)));

	// handle stop request
	EXPECT_CALL(*conn, publish(StrEq(control_channel.c_str()), StrEq("STOP")))
		// one time
		.Times(1)
		// and return back an integer reply
		.WillOnce(Return(getIntegerResult(1)));

	// initiate worker thread
	ASSERT_TRUE(BroadcastUtil::initialize(TEST_APP_ID.c_str(), TEST_NODE_ID.c_str()));
	// send a test message on our test channel
	BroadcastUtil::instance().publish(TEST_CHANNEL_NAME.c_str(), TEST_BROADCAST.c_str());

	// sleep at least one second to make sure worker thread starts execution
	sleep(1);
	BroadcastUtil::stopAll();
	delete conn;
}

/**
 * test worker thread logic to add and remove a channel subscription
 */
TEST(BroadcastUtilTest, controlAddRemoveSubscription) {
    // create a mock connection instance for worker thread
	MockRedisConnection* conn = new MockRedisConnection(NULL, 0);
	RedisConnectionTL::initializeTest(conn);

	// setup mock to expect command
	std::string control_channel = std::string(NAMESPACE_PREFIX) + ":" + TEST_APP_ID + ":CHANNELS:CONTROL:" + TEST_NODE_ID;
	std::string command_1 = std::string("SUBSCRIBE ") + control_channel;
	std::string command_2 = std::string("SUBSCRIBE ") + TEST_CHANNEL_NAME;
	std::string command_3 = std::string("UNSUBSCRIBE ") + TEST_CHANNEL_NAME;

	// handle worker thread clone
	EXPECT_CALL(*conn, clone())
	.WillRepeatedly(Return(*((RedisConnection*)conn)));

	// return connected when asked
	EXPECT_CALL(*conn, isConnected())
	.WillRepeatedly(Return(true));

	// return a control command to remove channel subscription once, other times just return blank result
	EXPECT_CALL(*conn, cmd(StrEq("")))
	.WillOnce(Return(getByDelay(RedisResult())))
	.WillOnce(Return(getByDelay(getControlCommand(
			(control_channel).c_str(),
			(std::string("REMOVE_CHANNEL ") + TEST_CHANNEL_NAME).c_str()))))
	.WillRepeatedly(Return(getByDelay(RedisResult())));

	// return a subscription control command after control channel has been subscribed
	EXPECT_CALL(*conn, cmd(StrEq(command_1.c_str())))
		// one time
		.Times(1)
		// and return back control command to subscribe
		.WillOnce(Return(getControlCommand(
				(control_channel).c_str(),
				(std::string("ADD_CHANNEL ") + TEST_CHANNEL_NAME).c_str())));

	// return a broadcast message to channel after it has been subscribed
	EXPECT_CALL(*conn, cmd(StrEq(command_2.c_str())))
		// one time
		.Times(1)
		.WillOnce(Return(getByDelay(getControlCommand(
				(TEST_CHANNEL_NAME).c_str(),
				(TEST_BROADCAST).c_str()))));

	// mock unsubscribe request handling
	EXPECT_CALL(*conn, cmd(StrEq(command_3.c_str())))
		// one time
		.Times(1)
		.WillOnce(Return(RedisResult()));


	// mock handling of request to stop
	EXPECT_CALL(*conn, publish(StrEq(control_channel.c_str()), StrEq("STOP")))
		// one time
		.Times(1)
		// and return back an integer reply
		.WillOnce(Return(getIntegerResult(1)));

	// initiate worker thread
	ASSERT_TRUE(BroadcastUtil::initialize(TEST_APP_ID.c_str(), TEST_NODE_ID.c_str()));
	// sleep at least one second to make sure worker thread starts execution
	sleep(1);
	BroadcastUtil::stopAll();
	delete conn;
}

/**
 * API test to add and remove subscriptions
 */
TEST(BroadcastUtilTest, commandAddRemoveSubscription) {
    // create a mock connection instance for worker thread
	MockRedisConnection* conn = new MockRedisConnection(NULL, 0);
	RedisConnectionTL::initializeTest(conn);

	// setup mock to expect command
	std::string control_channel = std::string(NAMESPACE_PREFIX) + ":" + TEST_APP_ID + ":CHANNELS:CONTROL:" + TEST_NODE_ID;
	std::string command_1 = std::string("SUBSCRIBE ") + control_channel;
	std::string command_2 = "ADD_CHANNEL " + TEST_CHANNEL_NAME;
	std::string command_3 = "REMOVE_CHANNEL " + TEST_CHANNEL_NAME;

	// handle worker thread clone
	EXPECT_CALL(*conn, clone())
	.WillRepeatedly(Return(*((RedisConnection*)conn)));

	// return connected when checked
	EXPECT_CALL(*conn, isConnected())
	.WillRepeatedly(Return(true));

	// return a blank command every 1 second when blocked on subscription messages
	EXPECT_CALL(*conn, cmd(StrEq("")))
	.WillRepeatedly(Return(getByDelay(RedisResult())));

	// return a blank command after control channel subscription
	EXPECT_CALL(*conn, cmd(StrEq(command_1.c_str())))
		// one time
		.Times(1)
		// and return back control command to subscribe
		.WillOnce(Return(getByDelay(RedisResult())));

	// verify that correct control command is sent to subscribe to a channel
	EXPECT_CALL(*conn, publish(StrEq(control_channel.c_str()), StrEq(command_2.c_str())))
		// one time
		.Times(1)
		// and return back an integer reply
		.WillOnce(Return(getIntegerResult(1)));

	// verify that correct control command is sent to unsubscribe from a channel
	EXPECT_CALL(*conn, publish(StrEq(control_channel.c_str()), StrEq(command_3.c_str())))
		// one time
		.Times(1)
		// and return back an integer reply
		.WillOnce(Return(getIntegerResult(1)));

	EXPECT_CALL(*conn, publish(StrEq(control_channel.c_str()), StrEq("STOP")))
		// one time
		.Times(1)
		// and return back an integer reply
		.WillOnce(Return(getIntegerResult(1)));

	// initiate worker thread
	ASSERT_TRUE(BroadcastUtil::initialize(TEST_APP_ID.c_str(), TEST_NODE_ID.c_str()));
	// subscribe to test channel
	ASSERT_EQ(BroadcastUtil::instance().addSubscription(TEST_CHANNEL_NAME.c_str(), test_call_back), 1);
	// unsubscribe from test channel
	ASSERT_EQ(BroadcastUtil::instance().removeSubscription(TEST_CHANNEL_NAME.c_str()), 1);
	// sleep at least one second to make sure worker thread starts execution
	sleep(1);
	BroadcastUtil::stopAll();
	delete conn;
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
