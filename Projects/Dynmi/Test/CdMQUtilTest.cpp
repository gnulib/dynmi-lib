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
#include <string>
#include <ctime>

using namespace ::testing;

static const std::string TEST_APP_ID = "999";
static const std::string TEST_TAG = "123";
static const std::string TEST_LOCKED_TAG = "999";
static const std::string TEST_CHANNEL_NAME = "test:channel";
static const std::string TEST_MESSAGE = "\"this is a test\"";
static const std::string TEST_LOCKED_PAYLOAD = "{\"tag\":\"" + TEST_LOCKED_TAG + "\",\"message\":\"" + "locked message" + "\"}";
static const std::string TEST_PAYLOAD = "{\"tag\":\"" + TEST_TAG + "\",\"message\":\"" + TEST_MESSAGE + "\"}";
const static std::string CDMQ = "CDMQ:APP:";
const static std::string CHANNEL_LOCK = ":LOCK:CHANNEL:";
const static std::string SESSION_LOCK = ":LOCK:SESSION:";


TEST(CdMQUtilTest, initialize) {
	MockRedisConnection* conn = new MockRedisConnection(NULL,0);
	RedisConnectionTL::initializeTest(conn);
	ASSERT_TRUE(CdMQUtil::initialize("",0));
	delete conn;
}

TEST(CdMQUtilTest, testMockInstanceUtil) {
	MockRedisConnection conn(NULL, 0);
	MockInstancesUtil* mIU = new MockInstancesUtil();
	InstancesUtil::initialize(mIU);
	EXPECT_CALL(*mIU, getFastLock(_, _, _, _))
		.Times(1)
		.WillOnce(Return(878));
	ASSERT_EQ(&(InstancesUtil::instance()), mIU);
	ASSERT_EQ(InstancesUtil::instance().getFastLock(conn, NULL, NULL, 1), 878);
	delete mIU;
}

TEST(CdMQUtilTest, enQueue) {
	MockRedisConnection* conn = new MockRedisConnection(NULL,0);
	MockInstancesUtil* mIU = new MockInstancesUtil();
	RedisConnectionTL::initializeTest(conn);
	InstancesUtil::initialize(mIU);

	EXPECT_CALL(*mIU, getFastLock(Ref(*conn), StrEq(TEST_APP_ID.c_str()), StrEq((CDMQ + TEST_APP_ID + CHANNEL_LOCK + TEST_CHANNEL_NAME).c_str()), Eq(10)))
		.Times(1)
		.WillOnce(Return(0));
	EXPECT_CALL(*mIU, releaseFastLock(Ref(*conn), StrEq(TEST_APP_ID.c_str()), StrEq((CDMQ + TEST_APP_ID + CHANNEL_LOCK + TEST_CHANNEL_NAME).c_str())))
		.Times(1)
		.WillOnce(Return(0));
//	Matcher<std::string> args[] = {Eq("RPUSH "), _, Eq(TEST_MESSAGE)};
//	std::string args[] = {"RPUSH ", "", TEST_MESSAGE};
//	EXPECT_CALL(*conn, cmdArgv(3,ElementsAre(Eq(std::string("RPUSH ")), _, Eq(TEST_MESSAGE))))
	EXPECT_CALL(*conn, cmdArgv(3,_))
//	.With(Args<1,1>(ElementsAreArray(args,3)))
	.Times(1)
	.WillOnce(Return(RedisResult()));

	CdMQUtil::initialize("",0);
	CdMQUtil::instance().enQueue(TEST_APP_ID, TEST_CHANNEL_NAME, TEST_MESSAGE, TEST_TAG);
	delete conn;
	delete mIU;
}

TEST(CdMQUtilTest, deQueue) {
	MockRedisConnection* conn = new MockRedisConnection(NULL,0);
	MockInstancesUtil* mIU = new MockInstancesUtil();
	RedisConnectionTL::initializeTest(conn);
	InstancesUtil::initialize(mIU);
	CdMQUtil::initialize("",0);

	EXPECT_CALL(*mIU, getFastLock(Ref(*conn), StrEq(TEST_APP_ID.c_str()), StrEq((CDMQ + TEST_APP_ID + CHANNEL_LOCK + TEST_CHANNEL_NAME).c_str()), Eq(10)))
		.Times(1)
		.WillOnce(Return(0));
	EXPECT_CALL(*mIU, getFastLock(Ref(*conn), StrEq(TEST_APP_ID.c_str()), StrEq((CDMQ + TEST_APP_ID + SESSION_LOCK + TEST_TAG).c_str()), Eq(5)))
		.Times(1)
		.WillOnce(Return(0));

	redisReply** values = new redisReply*[1];
	values[0] = getStringReply(TEST_PAYLOAD.c_str());
	EXPECT_CALL(*conn, cmd(StartsWith("LRANGE")))
	.Times(1)
	.WillOnce(Return(getArrayResult(1,values)));

	EXPECT_CALL(*conn, cmd(StartsWith("LSET")))
	.Times(1)
	.WillOnce(Return(RedisResult()));

	EXPECT_CALL(*conn, cmdArgv(4,_))
	.Times(1)
	.WillOnce(Return(RedisResult()));

	EXPECT_CALL(*mIU, releaseFastLock(Ref(*conn), StrEq(TEST_APP_ID.c_str()), StrEq((CDMQ + TEST_APP_ID + SESSION_LOCK + TEST_TAG).c_str())))
		.Times(1)
		.WillOnce(Return(0));
	EXPECT_CALL(*mIU, releaseFastLock(Ref(*conn), StrEq(TEST_APP_ID.c_str()), StrEq((CDMQ + TEST_APP_ID + CHANNEL_LOCK + TEST_CHANNEL_NAME).c_str())))
		.Times(1)
		.WillOnce(Return(0));

	// session lock is released only when message goes out of scope
	{
		CdMQMessage message = CdMQUtil::instance().deQueue(TEST_APP_ID, TEST_CHANNEL_NAME,5);
		ASSERT_TRUE(message.isValid());
		ASSERT_STREQ(message.getData().c_str(), TEST_MESSAGE.c_str());
	}
	delete conn;
	delete mIU;
}

TEST(CdMQUtilTest, deQueue2nd) {
	MockRedisConnection* conn = new MockRedisConnection(NULL,0);
	MockInstancesUtil* mIU = new MockInstancesUtil();
	RedisConnectionTL::initializeTest(conn);
	InstancesUtil::initialize(mIU);
	CdMQUtil::initialize("",0);

	EXPECT_CALL(*mIU, getFastLock(Ref(*conn), StrEq(TEST_APP_ID.c_str()), StrEq((CDMQ + TEST_APP_ID + CHANNEL_LOCK + TEST_CHANNEL_NAME).c_str()), Eq(10)))
		.Times(1)
		.WillOnce(Return(0));
	EXPECT_CALL(*mIU, getFastLock(Ref(*conn), StrEq(TEST_APP_ID.c_str()), StrEq((CDMQ + TEST_APP_ID + SESSION_LOCK + TEST_LOCKED_TAG).c_str()), Eq(5)))
		.Times(1)
		.WillOnce(Return(60));
	EXPECT_CALL(*mIU, getFastLock(Ref(*conn), StrEq(TEST_APP_ID.c_str()), StrEq((CDMQ + TEST_APP_ID + SESSION_LOCK + TEST_TAG).c_str()), Eq(5)))
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
	.WillOnce(Return(RedisResult()));

	EXPECT_CALL(*mIU, releaseFastLock(Ref(*conn), StrEq(TEST_APP_ID.c_str()), StrEq((CDMQ + TEST_APP_ID + SESSION_LOCK + TEST_TAG).c_str())))
		.Times(1)
		.WillOnce(Return(0));
	EXPECT_CALL(*mIU, releaseFastLock(Ref(*conn), StrEq(TEST_APP_ID.c_str()), StrEq((CDMQ + TEST_APP_ID + CHANNEL_LOCK + TEST_CHANNEL_NAME).c_str())))
		.Times(1)
		.WillOnce(Return(0));

	// session lock is released only when message goes out of scope
	{
		CdMQMessage message = CdMQUtil::instance().deQueue(TEST_APP_ID, TEST_CHANNEL_NAME,5);
		ASSERT_TRUE(message.isValid());
		ASSERT_STREQ(message.getData().c_str(), TEST_MESSAGE.c_str());
	}
	delete conn;
	delete mIU;
}

TEST(CdMQUtilTest, deQueueAllLocked) {
	MockRedisConnection* conn = new MockRedisConnection(NULL,0);
	MockInstancesUtil* mIU = new MockInstancesUtil();
	RedisConnectionTL::initializeTest(conn);
	InstancesUtil::initialize(mIU);
	CdMQUtil::initialize("",0);

	EXPECT_CALL(*mIU, getFastLock(Ref(*conn), StrEq(TEST_APP_ID.c_str()), StrEq((CDMQ + TEST_APP_ID + CHANNEL_LOCK + TEST_CHANNEL_NAME).c_str()), Eq(10)))
		.Times(1)
		.WillOnce(Return(0));
	EXPECT_CALL(*mIU, getFastLock(Ref(*conn), StrEq(TEST_APP_ID.c_str()), StrEq((CDMQ + TEST_APP_ID + SESSION_LOCK + TEST_LOCKED_TAG).c_str()), Eq(5)))
		.Times(1)
		.WillOnce(Return(60));
	EXPECT_CALL(*mIU, getFastLock(Ref(*conn), StrEq(TEST_APP_ID.c_str()), StrEq((CDMQ + TEST_APP_ID + SESSION_LOCK + TEST_TAG).c_str()), Eq(5)))
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

	EXPECT_CALL(*mIU, releaseFastLock(Ref(*conn), StrEq(TEST_APP_ID.c_str()), StrEq((CDMQ + TEST_APP_ID + SESSION_LOCK + TEST_TAG).c_str())))
		.Times(0);
	EXPECT_CALL(*mIU, releaseFastLock(Ref(*conn), StrEq(TEST_APP_ID.c_str()), StrEq((CDMQ + TEST_APP_ID + CHANNEL_LOCK + TEST_CHANNEL_NAME).c_str())))
		.Times(1)
		.WillOnce(Return(0));

	// session lock is released only when message goes out of scope
	{
		CdMQMessage message = CdMQUtil::instance().deQueue(TEST_APP_ID, TEST_CHANNEL_NAME,5);
		ASSERT_FALSE(message.isValid());
		ASSERT_STRNE(message.getData().c_str(), TEST_MESSAGE.c_str());
	}
	delete conn;
	delete mIU;
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
