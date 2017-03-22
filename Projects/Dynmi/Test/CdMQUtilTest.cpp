/*
 * CdMQUtilTest.cpp
 *
 *  Created on: Mar 20, 2017
 *      Author: bhadoria
 */

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "Dynmi/BroadcastUtil.hpp"
#include "Dynmi/CdMQUtil.hpp"
#include "Dynmi/CdMQMessage.hpp"
#include "CdMQPayload.hpp"
#include "Dynmi/RedisResult.hpp"
#include "Dynmi/RedisConnectionTL.hpp"
#include "Dynmi/DynmiGlobals.hpp"
#include "RedisReplyFixtures.hpp"
#include "MockRedisConnection.hpp"
#include "MockInstancesUtil.hpp"
#include "MockBroadcastUtil.hpp"
#include <string>
#include <ctime>

using namespace ::testing;

static const std::string TEST_APP_ID = "999";
static const std::string TEST_TAG = "123";
static const std::string TEST_LOCKED_TAG = "999";
static const std::string TEST_CHANNEL_NAME = "test:channel";
static const std::string TEST_MESSAGE = "\"this is a test\"";
static const std::string TEST_LOCKED_PAYLOAD = "{\"tag\":\"" + TEST_LOCKED_TAG + "\",\"channel\":\"" + TEST_CHANNEL_NAME + "\",\"message\":\"" + "locked message" + "\"}";
static const std::string TEST_PAYLOAD = "{\"tag\":\"" + TEST_TAG + + "\",\"channel\":\"" + TEST_CHANNEL_NAME + "\",\"message\":\"" + TEST_MESSAGE + "\"}";
static const std::string TEST_UNTAG_PAYLOAD = "{\"tag\":\"\",\"channel\":\"" + TEST_CHANNEL_NAME + "\",\"message\":\"" + TEST_MESSAGE + "\"}";
const static std::string CDMQ = "CDMQ:APP:";
const static std::string CHANNEL_LOCK = ":LOCK:CHANNEL:";
const static std::string SESSION_LOCK = ":LOCK:SESSION:";
static const std::string CHANNEL_ACTIVE = "CHANNEL_ACTIVE";


TEST(CdMQUtilTest, initialize) {
	MockRedisConnection* conn = new MockRedisConnection(NULL,0);
	RedisConnectionTL::initializeTest(conn);
	ASSERT_TRUE(CdMQUtil::initialize(TEST_APP_ID,"",0));
	delete conn;
}

TEST(CdMQUtilTest, testMockInstanceUtil) {
	MockRedisConnection conn(NULL, 0);
	MockInstancesUtil* mIU = new MockInstancesUtil();
	InstancesUtil::initialize(mIU);
	EXPECT_CALL(*mIU, getFastLock(_, _, _))
		.Times(1)
		.WillOnce(Return(878));
	ASSERT_EQ(&(InstancesUtil::instance()), mIU);
	ASSERT_EQ(InstancesUtil::instance().getFastLock(NULL, NULL, 1), 878);
	delete mIU;
}

TEST(CdMQUtilTest, enQueue) {
	MockRedisConnection* conn = new MockRedisConnection(NULL,0);
	RedisConnectionTL::initializeTest(conn);
	MockInstancesUtil* mIU = new MockInstancesUtil();
	InstancesUtil::initialize(mIU);
	MockBroadcastUtil* mBU = new MockBroadcastUtil();
	BroadcastUtil::initialize(mBU);

	EXPECT_CALL(*mIU, getFastLock(StrEq(TEST_APP_ID.c_str()), StrEq((CDMQ + TEST_APP_ID + CHANNEL_LOCK + TEST_CHANNEL_NAME).c_str()), Eq(10)))
		.Times(1)
		.WillOnce(Return(0));

	EXPECT_CALL(*mBU, publish(HasSubstr(CHANNEL_ACTIVE.c_str()), StrEq(TEST_CHANNEL_NAME.c_str())))
	.Times(1)
	.WillOnce(Return(1));

	EXPECT_CALL(*conn, cmdArgv(3,_))
	.Times(1)
	.WillOnce(Return(getIntegerResult(1)));

	EXPECT_CALL(*mIU, releaseFastLock(StrEq(TEST_APP_ID.c_str()), StrEq((CDMQ + TEST_APP_ID + CHANNEL_LOCK + TEST_CHANNEL_NAME).c_str())))
		.Times(1)
		.WillOnce(Return(0));

	CdMQUtil::initialize(TEST_APP_ID,"",0);
	CdMQUtil::instance().enQueue(TEST_APP_ID, TEST_CHANNEL_NAME, TEST_MESSAGE, TEST_TAG);
	delete conn;
	delete mIU;
	delete mBU;
}

TEST(CdMQUtilTest, enQueueNoTag) {
	MockRedisConnection* conn = new MockRedisConnection(NULL,0);
	RedisConnectionTL::initializeTest(conn);
	MockInstancesUtil* mIU = new MockInstancesUtil();
	InstancesUtil::initialize(mIU);
	MockBroadcastUtil* mBU = new MockBroadcastUtil();
	BroadcastUtil::initialize(mBU);

	EXPECT_CALL(*mIU, getFastLock(StrEq(TEST_APP_ID.c_str()), StrEq((CDMQ + TEST_APP_ID + CHANNEL_LOCK + TEST_CHANNEL_NAME).c_str()), Eq(10)))
		.Times(1)
		.WillOnce(Return(0));

	EXPECT_CALL(*mBU, publish(HasSubstr(CHANNEL_ACTIVE.c_str()), StrEq(TEST_CHANNEL_NAME.c_str())))
	.Times(1)
	.WillOnce(Return(1));

	EXPECT_CALL(*conn, cmdArgv(3,_))
	.Times(1)
	.WillOnce(Return(getIntegerResult(1)));

	EXPECT_CALL(*mIU, releaseFastLock(StrEq(TEST_APP_ID.c_str()), StrEq((CDMQ + TEST_APP_ID + CHANNEL_LOCK + TEST_CHANNEL_NAME).c_str())))
		.Times(1)
		.WillOnce(Return(0));

	CdMQUtil::initialize(TEST_APP_ID,"",0);
	CdMQUtil::instance().enQueue(TEST_APP_ID, TEST_CHANNEL_NAME, TEST_MESSAGE);
	delete conn;
	delete mIU;
	delete mBU;
}

TEST(CdMQUtilTest, deQueue) {
	MockRedisConnection* conn = new MockRedisConnection(NULL,0);
	MockInstancesUtil* mIU = new MockInstancesUtil();
	RedisConnectionTL::initializeTest(conn);
	InstancesUtil::initialize(mIU);
	CdMQUtil::initialize(TEST_APP_ID,"",0);
	MockBroadcastUtil* mBU = new MockBroadcastUtil();
	BroadcastUtil::initialize(mBU);

	{
		InSequence dummy;

		// first we want to acquire lock for channel leader selection (only 1 app instance should get next message)
		EXPECT_CALL(*mIU, getFastLock(StrEq(TEST_APP_ID.c_str()), StrEq((CDMQ + TEST_APP_ID + CHANNEL_LOCK + TEST_CHANNEL_NAME).c_str()), Eq(10)))
			.Times(1)
			.WillOnce(Return(0));

		// then we want to get all current messages in the queue
		redisReply** values = new redisReply*[1];
		values[0] = getStringReply(TEST_PAYLOAD.c_str());

		EXPECT_CALL(*conn, cmd(ContainsRegex("LRANGE .*CHANNEL.*")))
		.Times(1)
		.WillOnce(Return(getArrayResult(1,values)));

		// we want to attempt getting lock on the session/tag of the message
		EXPECT_CALL(*mIU, getFastLock(StrEq(TEST_APP_ID.c_str()), StrEq((CDMQ + TEST_APP_ID + SESSION_LOCK + TEST_TAG).c_str()), Eq(5)))
			.Times(1)
			.WillOnce(Return(0));

		// we mark the message in queue with TOMBSTONE
		EXPECT_CALL(*conn, cmd(StartsWith("LSET")))
		.Times(1)
		.WillOnce(Return(RedisResult()));

		// delete the actual TOMBSTONE
		EXPECT_CALL(*conn, cmdArgv(4,_))
		.Times(1)
		.WillOnce(Return(getIntegerResult(1)));

		// release lock on the channel
		EXPECT_CALL(*mIU, releaseFastLock(StrEq(TEST_APP_ID.c_str()), StrEq((CDMQ + TEST_APP_ID + CHANNEL_LOCK + TEST_CHANNEL_NAME).c_str())))
			.Times(1)
			.WillOnce(Return(0));

		// release session/tag lock once message is out of scope
		EXPECT_CALL(*mIU, releaseFastLock(StrEq(TEST_APP_ID.c_str()), StrEq((CDMQ + TEST_APP_ID + SESSION_LOCK + TEST_TAG).c_str())))
			.Times(1)
			.WillOnce(Return(0));

		// publish channel active after current message processing finishes
		EXPECT_CALL(*mBU, publish(HasSubstr(CHANNEL_ACTIVE.c_str()), StrEq(TEST_CHANNEL_NAME.c_str())))
		.Times(1)
		.WillOnce(Return(1));
	}


	// session lock is released only when message goes out of scope
	{
		CdMQMessage message = CdMQUtil::instance().deQueue(TEST_CHANNEL_NAME,5);
		ASSERT_TRUE(message.isValid());
		ASSERT_STREQ(message.getData().c_str(), TEST_MESSAGE.c_str());
	}
	delete conn;
	delete mIU;
	delete mBU;
}

TEST(CdMQUtilTest, deQueueNoTag) {
	MockRedisConnection* conn = new MockRedisConnection(NULL,0);
	MockInstancesUtil* mIU = new MockInstancesUtil();
	RedisConnectionTL::initializeTest(conn);
	InstancesUtil::initialize(mIU);
	CdMQUtil::initialize(TEST_APP_ID,"",0);
	MockBroadcastUtil* mBU = new MockBroadcastUtil();
	BroadcastUtil::initialize(mBU);

	{
		InSequence dummy;

		// first we want to acquire lock for channel leader selection (only 1 app instance should get next message)
		EXPECT_CALL(*mIU, getFastLock(StrEq(TEST_APP_ID.c_str()), StrEq((CDMQ + TEST_APP_ID + CHANNEL_LOCK + TEST_CHANNEL_NAME).c_str()), Eq(10)))
			.Times(1)
			.WillOnce(Return(0));

		// then we want to get all current messages in the queue
		redisReply** values = new redisReply*[1];
		values[0] = getStringReply(TEST_UNTAG_PAYLOAD.c_str());

		EXPECT_CALL(*conn, cmd(ContainsRegex("LRANGE .*CHANNEL.*")))
		.Times(1)
		.WillOnce(Return(getArrayResult(1,values)));

		// we should not attempt getting lock on the session/tag for untagged message
		EXPECT_CALL(*mIU, getFastLock(StrEq(TEST_APP_ID.c_str()), StartsWith((CDMQ + TEST_APP_ID + SESSION_LOCK).c_str()), Eq(5)))
			.Times(0);

		// we mark the message in queue with TOMBSTONE
		EXPECT_CALL(*conn, cmd(StartsWith("LSET")))
		.Times(1)
		.WillOnce(Return(RedisResult()));

		// delete the actual TOMBSTONE
		EXPECT_CALL(*conn, cmdArgv(4,_))
		.Times(1)
		.WillOnce(Return(getIntegerResult(1)));

		// release lock on the channel
		EXPECT_CALL(*mIU, releaseFastLock(StrEq(TEST_APP_ID.c_str()), StrEq((CDMQ + TEST_APP_ID + CHANNEL_LOCK + TEST_CHANNEL_NAME).c_str())))
			.Times(1)
			.WillOnce(Return(0));

		// no lock to release when untagged message is out of scope
		EXPECT_CALL(*mIU, releaseFastLock(StrEq(TEST_APP_ID.c_str()), StartsWith((CDMQ + TEST_APP_ID + SESSION_LOCK).c_str())))
			.Times(0);

		// publish channel active after current message processing finishes
		EXPECT_CALL(*mBU, publish(HasSubstr(CHANNEL_ACTIVE.c_str()), StrEq(TEST_CHANNEL_NAME.c_str())))
		.Times(1)
		.WillOnce(Return(1));
	}


	// session lock is released only when message goes out of scope
	{
		CdMQMessage message = CdMQUtil::instance().deQueue(TEST_CHANNEL_NAME,5);
		ASSERT_TRUE(message.isValid());
		ASSERT_STREQ(message.getData().c_str(), TEST_MESSAGE.c_str());
	}
	delete conn;
	delete mIU;
	delete mBU;
}

TEST(CdMQUtilTest, deQueue2nd) {
	MockRedisConnection* conn = new MockRedisConnection(NULL,0);
	MockInstancesUtil* mIU = new MockInstancesUtil();
	RedisConnectionTL::initializeTest(conn);
	InstancesUtil::initialize(mIU);
	CdMQUtil::initialize(TEST_APP_ID,"",0);
	MockBroadcastUtil* mBU = new MockBroadcastUtil();
	BroadcastUtil::initialize(mBU);

	EXPECT_CALL(*mIU, getFastLock(StrEq(TEST_APP_ID.c_str()), StrEq((CDMQ + TEST_APP_ID + CHANNEL_LOCK + TEST_CHANNEL_NAME).c_str()), Eq(10)))
		.Times(1)
		.WillOnce(Return(0));
	EXPECT_CALL(*mIU, getFastLock(StrEq(TEST_APP_ID.c_str()), StrEq((CDMQ + TEST_APP_ID + SESSION_LOCK + TEST_LOCKED_TAG).c_str()), Eq(5)))
		.Times(1)
		.WillOnce(Return(60));
	EXPECT_CALL(*mIU, getFastLock(StrEq(TEST_APP_ID.c_str()), StrEq((CDMQ + TEST_APP_ID + SESSION_LOCK + TEST_TAG).c_str()), Eq(5)))
		.Times(1)
		.WillOnce(Return(0));

	redisReply** values = new redisReply*[2];
	values[0] = getStringReply(TEST_LOCKED_PAYLOAD.c_str());
	values[1] = getStringReply(TEST_PAYLOAD.c_str());
	EXPECT_CALL(*conn, cmd(StartsWith("LRANGE")))
	.Times(1)
	.WillOnce(Return(getArrayResult(2,values)));

	EXPECT_CALL(*conn, cmd(StartsWith("LSET")))
	.Times(1)
	.WillOnce(Return(RedisResult()));

	EXPECT_CALL(*conn, cmdArgv(4,_))
	.Times(1)
	.WillOnce(Return(getIntegerResult(1)));

	EXPECT_CALL(*mIU, releaseFastLock(StrEq(TEST_APP_ID.c_str()), StrEq((CDMQ + TEST_APP_ID + SESSION_LOCK + TEST_TAG).c_str())))
		.Times(1)
		.WillOnce(Return(0));

	EXPECT_CALL(*mIU, releaseFastLock(StrEq(TEST_APP_ID.c_str()), StrEq((CDMQ + TEST_APP_ID + CHANNEL_LOCK + TEST_CHANNEL_NAME).c_str())))
		.Times(1)
		.WillOnce(Return(0));

	EXPECT_CALL(*mBU, publish(HasSubstr(CHANNEL_ACTIVE.c_str()), StrEq(TEST_CHANNEL_NAME.c_str())))
	.Times(1)
	.WillOnce(Return(1));

	// session lock is released only when message goes out of scope
	{
		CdMQMessage message = CdMQUtil::instance().deQueue(TEST_CHANNEL_NAME,5);
		ASSERT_TRUE(message.isValid());
		ASSERT_STREQ(message.getData().c_str(), TEST_MESSAGE.c_str());
	}
	delete conn;
	delete mIU;
	delete mBU;
}

TEST(CdMQUtilTest, deQueueAllLocked) {
	MockRedisConnection* conn = new MockRedisConnection(NULL,0);
	MockInstancesUtil* mIU = new MockInstancesUtil();
	RedisConnectionTL::initializeTest(conn);
	InstancesUtil::initialize(mIU);
	CdMQUtil::initialize(TEST_APP_ID,"",0);
	MockBroadcastUtil* mBU = new MockBroadcastUtil();
	BroadcastUtil::initialize(mBU);

	EXPECT_CALL(*mIU, getFastLock(StrEq(TEST_APP_ID.c_str()), StrEq((CDMQ + TEST_APP_ID + CHANNEL_LOCK + TEST_CHANNEL_NAME).c_str()), Eq(10)))
		.Times(1)
		.WillOnce(Return(0));
	EXPECT_CALL(*mIU, getFastLock(StrEq(TEST_APP_ID.c_str()), StrEq((CDMQ + TEST_APP_ID + SESSION_LOCK + TEST_LOCKED_TAG).c_str()), Eq(5)))
		.Times(1)
		.WillOnce(Return(60));
	EXPECT_CALL(*mIU, getFastLock(StrEq(TEST_APP_ID.c_str()), StrEq((CDMQ + TEST_APP_ID + SESSION_LOCK + TEST_TAG).c_str()), Eq(5)))
		.Times(1)
		.WillOnce(Return(60));

	redisReply** values = new redisReply*[2];
	values[0] = getStringReply(TEST_LOCKED_PAYLOAD.c_str());
	values[1] = getStringReply(TEST_PAYLOAD.c_str());
	EXPECT_CALL(*conn, cmd(StartsWith("LRANGE")))
	.Times(1)
	.WillOnce(Return(getArrayResult(2,values)));

	EXPECT_CALL(*conn, cmd(StartsWith("LSET")))
	.Times(0);

	EXPECT_CALL(*conn, cmdArgv(4,_))
	.Times(0);

	EXPECT_CALL(*mIU, releaseFastLock(StrEq(TEST_APP_ID.c_str()), StrEq((CDMQ + TEST_APP_ID + SESSION_LOCK + TEST_TAG).c_str())))
		.Times(0);
	EXPECT_CALL(*mIU, releaseFastLock(StrEq(TEST_APP_ID.c_str()), StrEq((CDMQ + TEST_APP_ID + CHANNEL_LOCK + TEST_CHANNEL_NAME).c_str())))
		.Times(1)
		.WillOnce(Return(0));

	EXPECT_CALL(*mBU, publish(HasSubstr(CHANNEL_ACTIVE.c_str()), StrEq(TEST_CHANNEL_NAME.c_str())))
	.Times(0);

	// session lock is released only when message goes out of scope
	{
		CdMQMessage message = CdMQUtil::instance().deQueue(TEST_CHANNEL_NAME,5);
		ASSERT_FALSE(message.isValid());
		ASSERT_STRNE(message.getData().c_str(), TEST_MESSAGE.c_str());
	}
	delete conn;
	delete mIU;
	delete mBU;
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
