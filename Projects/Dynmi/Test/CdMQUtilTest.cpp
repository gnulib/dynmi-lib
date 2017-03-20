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
static const std::string TEST_TAG = "1";
static const std::string TEST_CHANNEL_NAME = "test:channel";
static const std::string TEST_MESSAGE = "\"this is a test\"";
const static std::string CDMQ = "CDMQ:APP:";
const static std::string QUEUE = ":QUEUE:";
const static std::string CHANNEL_QUEUE = QUEUE + "CHANNEL:";
const static std::string CHANNEL_LOCK = ":LOCK:CHANNEL:";
const static std::string CHANNEL = ":CHANNEL:";

TEST(CdMQUtilTest, initialize) {
	MockRedisConnection* conn = new MockRedisConnection(NULL,0);
	RedisConnectionTL::initializeTest(conn);
	ASSERT_TRUE(CdMQUtil::initialize("",0));
	delete conn;
}

TEST(CdMQUtilTest, enQueue) {
	MockRedisConnection* conn = new MockRedisConnection(NULL,0);
	MockInstancesUtil* mIU = new MockInstancesUtil();
	RedisConnectionTL::initializeTest(conn);
	InstancesUtil::initialize(mIU);

//	EXPECT_CALL(*conn, isConnected())
//	.Times(1)
//	.WillOnce(Return(true));

//	EXPECT_CALL(*mIU, getFastLock(_, StrEq(TEST_APP_ID.c_str()), StrEq((CDMQ + TEST_APP_ID + CHANNEL_LOCK + TEST_CHANNEL_NAME).c_str()), 10))
//	EXPECT_CALL(*mIU, getFastLock(Ref(*conn), StrEq(TEST_APP_ID.c_str()), StrEq((CDMQ + TEST_APP_ID + CHANNEL_LOCK + TEST_CHANNEL_NAME).c_str()), Eq(10)))
//	EXPECT_CALL(*mIU, getFastLock(An<RedisConnection&>(), StrEq(TEST_APP_ID.c_str()), StrEq((CDMQ + TEST_APP_ID + CHANNEL_LOCK + TEST_CHANNEL_NAME).c_str()), Eq(10)))
	EXPECT_CALL(*mIU, getFastLock(_, _, _, _))
		.Times(1)
		.WillOnce(Return(0));

//	EXPECT_CALL(*conn, cmdArgv(3, NotNull()))
//	.Times(1)
//	.WillOnce(Return(RedisResult()));

	CdMQUtil::initialize("",0);
	CdMQUtil::instance().enQueue(TEST_APP_ID, TEST_CHANNEL_NAME, TEST_MESSAGE, TEST_TAG);
//	delete conn;
	delete mIU;
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
