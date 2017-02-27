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
	MOCK_METHOD2(publish, RedisResult(const char *, const char *));
};

TEST(InstancesUtilTest, getNewInstanceIdNonIntegerResultType) {
    // create a mock connection instance
	MockRedisConnection conn(NULL, 0);

	// setup mock to expect command
	EXPECT_CALL(conn, isConnected())
	.Times(1)
	.WillOnce(Return(true));

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
	EXPECT_CALL(conn, isConnected())
	.Times(1)
	.WillOnce(Return(true));

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
	std::string command = std::string("INCR ") + NAMESPACE_PREFIX + ":" + TEST_APP_ID + ":COUNTERS:NODES";
	EXPECT_CALL(conn, isConnected())
	.Times(1)
	.WillOnce(Return(true));

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
	EXPECT_CALL(conn, isConnected())
	.Times(1)
	.WillOnce(Return(true));

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
	std::string command1 = std::string("HMSET ") + NAMESPACE_PREFIX
							+ ":" + TEST_APP_ID + ":INSTANCE:" + TEST_NODE_ID
							+ ":ADDRESS HOST " + TEST_HOST + " PORT " + TEST_PORT;
	std::string command2 = std::string("EXPIRE ") + NAMESPACE_PREFIX
							+ ":" + TEST_APP_ID + ":INSTANCE:" + TEST_NODE_ID
							+ ":ADDRESS 20";
	std::string command3 = std::string("SADD ") + NAMESPACE_PREFIX
			+ ":" + TEST_APP_ID + ":INSTANCES " + TEST_NODE_ID;

	std::string channelInstanceUp = std::string(NAMESPACE_PREFIX)
			+ ":" + TEST_APP_ID + ":CHANNELS:INSTANCE_UP";
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
		EXPECT_CALL(conn, publish(StrEq(channelInstanceUp.c_str()), StrEq(TEST_NODE_ID.c_str())))
		.Times(1)
		.WillOnce(Return(getIntegerResult(1)));
	}
	// call the utility method to publish node's address
	// and expect 1 node as subscriber
	ASSERT_EQ(InstancesUtil::publishNodeDetails(conn, TEST_APP_ID.c_str(), std::atoi(TEST_NODE_ID.c_str()), TEST_HOST.c_str(), std::atoi(TEST_PORT.c_str()), 20), 1);
}

TEST(InstancesUtilTest, removeNodeDetailsCommandSequence) {
    // create a mock connection instance
	MockRedisConnection conn(NULL, 0);
	// first publish node down to all active instances
	std::string channelInstanceDown = std::string(NAMESPACE_PREFIX)
			+ ":" + TEST_APP_ID + ":CHANNELS:INSTANCE_DOWN";
	// then remove node's ID from set of active instances
	std::string command2 = std::string("SREM ") + NAMESPACE_PREFIX
			+ ":" + TEST_APP_ID + ":INSTANCES " + TEST_NODE_ID;
	// finally remove node's address details after 10 secs
	std::string command3 = std::string("DEL ") + NAMESPACE_PREFIX
							+ ":" + TEST_APP_ID + ":INSTANCE:" + TEST_NODE_ID
							+ ":ADDRESS";

	// expect following calls in sequence
	{
		InSequence dummy;
		EXPECT_CALL(conn, isConnected())
		.Times(1)
		.WillOnce(Return(true));
		EXPECT_CALL(conn, publish(StrEq(channelInstanceDown.c_str()), StrEq(TEST_NODE_ID.c_str())))
		.Times(1)
		.WillOnce(Return(getIntegerResult(1)));
		EXPECT_CALL(conn, cmd(StrEq(command2.c_str())))
		.Times(1)
		.WillOnce(Return(getIntegerResult(1)));
		EXPECT_CALL(conn, cmd(StrEq(command3.c_str())))
		.Times(1)
		.WillOnce(Return(RedisResult()));
	}
	// call the utility method to publish node's address
	// and expect 1 node as subscriber
	ASSERT_EQ(InstancesUtil::removeNodeDetails(conn, TEST_APP_ID.c_str(), std::atoi(TEST_NODE_ID.c_str())), 0);
}

/**
 * get a fast lock on system as following:
 *	1.	execute "SETNX <mutex key> <current Unix timestamp + mutex timeout + 1>"
 *		on the key for the mutex name
 *	2.	if return value is "1", then proceed with operation, and then delete the key
 *	3.	if return value is "0", then
 *		a.	get mutex key value (timestamp of expiry)
 *		b.	if it is more than current time, then wait (mutex in use)
 *		c.	otherwise try following:
 *			1)	execute "GETSET <mutex key> <current Unix timestamp + mutex timeout + 1>"
 *			2)	check the return value to see if this is same as old expired value
 *			3)	if return value is not same as original expired then some other instance
 *				got the mutex, and this instance needs to wait (or abort)
 *			4)	otherwise (if return value is same as expired), then instance successfully
 *				acquired the mutex, proceed with operation
 */
TEST(InstancesUtilTest, fastLockCommandSequenceStep1Lock) {
    // create a mock connection instance
	MockRedisConnection conn(NULL, 0);

	std::string key = std::string(NAMESPACE_PREFIX)
			+ ":" + TEST_APP_ID + ":LOCKS:" + TEST_LOCK_NAME;
	int ttl = 10;

	// first try to acquire lock
	std::string command1 = std::string("SETNX ") + key;
	// then set lock expiry when we lock successfully
#if __cplusplus >= 201103L
	std::string command2 = std::string("EXPIRE ") + key + " " + std::to_string(ttl+1);
#else
	std::string command2 = std::string("EXPIRE ") + key + " "
			+ static_cast<std::ostringstream*>( &(std::ostringstream() << (ttl + 1)) )->str();
#endif
	{
		InSequence dummy;
		EXPECT_CALL(conn, isConnected())
		.Times(1)
		.WillOnce(Return(true));
		EXPECT_CALL(conn, cmd(StartsWith(command1.c_str())))
		.Times(1)
		.WillOnce(Return(getIntegerResult(1)));
		EXPECT_CALL(conn, cmd(StrEq(command2.c_str())))
		.Times(1)
		.WillOnce(Return(getIntegerResult(1)));
	}
	// we expect a return value 0/success
	ASSERT_EQ(InstancesUtil::getFastLock(conn, TEST_APP_ID.c_str(), TEST_LOCK_NAME.c_str(), ttl), 0);
}

TEST(InstancesUtilTest, fastLockCommandSequenceStep2Busy) {
    // create a mock connection instance
	MockRedisConnection conn(NULL, 0);

	std::string key = std::string(NAMESPACE_PREFIX)
			+ ":" + TEST_APP_ID + ":LOCKS:" + TEST_LOCK_NAME;
	std::time_t now = std::time(0);
	int ttl = 10;

	// first try to acquire lock
	std::string command1 = std::string("SETNX ") + key;
	// then try to read expiry
	std::string command2 = std::string("GET ") + key;
#if __cplusplus >= 201103L
	std::string oldExpiry = std::string("EXPIRE ") + key + " " + std::to_string(now + ttl + 1);
#else
	std::string oldExpiry = static_cast<std::ostringstream*>( &(std::ostringstream() << (now + ttl + 1)) )->str();
#endif
	{
		InSequence dummy;
		EXPECT_CALL(conn, isConnected())
		.Times(1)
		.WillOnce(Return(true));
		EXPECT_CALL(conn, cmd(StartsWith(command1.c_str())))
		.Times(1)
		.WillOnce(Return(getIntegerResult(0)));
		EXPECT_CALL(conn, cmd(StartsWith(command2.c_str())))
		.Times(1)
		// mock return value to send an expiry which is ttl+1 sec in future
		.WillOnce(Return(getStringResult(oldExpiry.c_str())));
	}
	// we expect a return value that has remaining time for lock is busy
	ASSERT_EQ(InstancesUtil::getFastLock(conn, TEST_APP_ID.c_str(), TEST_LOCK_NAME.c_str(), ttl), ttl+1);
}

TEST(InstancesUtilTest, fastLockCommandSequenceStep3Lost) {
    // create a mock connection instance
	MockRedisConnection conn(NULL, 0);

	std::string key = std::string(NAMESPACE_PREFIX)
			+ ":" + TEST_APP_ID + ":LOCKS:" + TEST_LOCK_NAME;
	std::time_t now = std::time(0);
	int ttl = 10;

	// first try to acquire lock
	std::string command1 = std::string("SETNX ") + key;
	// then try to read expiry
	std::string command2 = std::string("GET ") + key;
	// then retry getting the lock
	std::string command3 = std::string("GETSET ") + key;
	// revert back lock value when lost
	std::string command4 = std::string("SET ") + key;
#if __cplusplus >= 201103L
	std::string oldExpiry = std::string("EXPIRE ") + key + " " + std::to_string(now - 1);
	std::string newExpiry = std::string("EXPIRE ") + key + " " + std::to_string(now + ttl + 1);
#else
	std::string oldExpiry = static_cast<std::ostringstream*>( &(std::ostringstream() << (now - 1)) )->str();
	std::string newExpiry = static_cast<std::ostringstream*>( &(std::ostringstream() << (now + ttl + 1)) )->str();
#endif
	{
		InSequence dummy;
		EXPECT_CALL(conn, isConnected())
		.Times(1)
		.WillOnce(Return(true));

		EXPECT_CALL(conn, cmd(StartsWith(command1.c_str())))
		.Times(1)
		.WillOnce(Return(getIntegerResult(0)));

		EXPECT_CALL(conn, cmd(StartsWith(command2.c_str())))
		.Times(1)
		// mock return value to send old expiry which is in past
		.WillOnce(Return(getStringResult(oldExpiry.c_str())));

		EXPECT_CALL(conn, cmd(StartsWith(command3.c_str())))
		.Times(1)
		// mock return value to send new expiry which is ttl+1 sec in future
		.WillOnce(Return(getStringResult(newExpiry.c_str())));

		EXPECT_CALL(conn, cmd(StartsWith(command4.c_str())))
		.Times(1)
		.WillOnce(Return(RedisResult()));
	}
	// we expect a return value that has remaining time for lock is busy
	ASSERT_EQ(InstancesUtil::getFastLock(conn, TEST_APP_ID.c_str(), TEST_LOCK_NAME.c_str(), ttl), ttl+1);
}

TEST(InstancesUtilTest, fastLockCommandSequenceStep3Won) {
    // create a mock connection instance
	MockRedisConnection conn(NULL, 0);

	std::string key = std::string(NAMESPACE_PREFIX)
			+ ":" + TEST_APP_ID + ":LOCKS:" + TEST_LOCK_NAME;
	std::time_t now = std::time(0);
	int ttl = 10;

	// first try to acquire lock
	std::string command1 = std::string("SETNX ") + key;
	// then try to read expiry
	std::string command2 = std::string("GET ") + key;
	// then retry getting the lock
	std::string command3 = std::string("GETSET ") + key;
#if __cplusplus >= 201103L
	std::string oldExpiry = std::string("EXPIRE ") + key + " " + std::to_string(now - 1);
	// set lock expiry after we won it
	std::string command4 = std::string("EXPIRE ") + key + " " + std::to_string(ttl+1);
#else
	std::string oldExpiry = static_cast<std::ostringstream*>( &(std::ostringstream() << (now - 1)) )->str();
	// set lock expiry after we won it
	std::string command4 = std::string("EXPIRE ") + key + " "
			+ static_cast<std::ostringstream*>( &(std::ostringstream() << (ttl + 1)) )->str();
#endif
	{
		InSequence dummy;
		EXPECT_CALL(conn, isConnected())
		.Times(1)
		.WillOnce(Return(true));

		EXPECT_CALL(conn, cmd(StartsWith(command1.c_str())))
		.Times(1)
		.WillOnce(Return(getIntegerResult(0)));

		EXPECT_CALL(conn, cmd(StartsWith(command2.c_str())))
		.Times(1)
		// mock return value to send old expiry which is in past
		.WillOnce(Return(getStringResult(oldExpiry.c_str())));

		EXPECT_CALL(conn, cmd(StartsWith(command3.c_str())))
		.Times(1)
		// mock return value to send same old expiry
		.WillOnce(Return(getStringResult(oldExpiry.c_str())));

		EXPECT_CALL(conn, cmd(StrEq(command4.c_str())))
		.Times(1)
		.WillOnce(Return(getIntegerResult(1)));
}
	// we expect a return value 0/success
	ASSERT_EQ(InstancesUtil::getFastLock(conn, TEST_APP_ID.c_str(), TEST_LOCK_NAME.c_str(), ttl), 0);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
