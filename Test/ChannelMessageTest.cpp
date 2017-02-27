/*
 * ChannelMessageTest.cpp
 *
 *  Created on: Feb 26, 2017
 *      Author: bhadoria
 */

#include "gtest/gtest.h"
#include "ChannelMessage.hpp"
#include "RedisReplyFixtures.hpp"

static const char * TEST_CHANNEL = "test:channel";
static const char * TEST_MESSAGE = "a test message";

TEST(ChannelMessageTest, instanceFromErrorResponse) {
	ChannelMessage message = ChannelMessage::from(getErrorResult());
	ASSERT_EQ(message.getType(), channelMessage::ERROR);
	ASSERT_EQ(message.getChannelName().size(),0);
	ASSERT_EQ(message.getMessage().size(),0);
}

TEST(ChannelMessageTest, instanceFromNonArray) {
	ChannelMessage message = ChannelMessage::from(getStringResult(TEST_MESSAGE));
	ASSERT_EQ(message.getType(), channelMessage::ERROR);
	ASSERT_EQ(message.getChannelName().size(),0);
	ASSERT_EQ(message.getMessage().size(),0);
}

TEST(ChannelMessageTest, instanceFromIncomplete) {
	redisReply** values = new redisReply*[2];
	values[0] = getStringReply("message");
	values[1] = getStringReply(TEST_CHANNEL);
	ChannelMessage message = ChannelMessage::from(getArrayResult(2, values));
	ASSERT_EQ(message.getType(), channelMessage::ERROR);
	ASSERT_EQ(message.getChannelName().size(),0);
	ASSERT_EQ(message.getMessage().size(),0);
}

TEST(ChannelMessageTest, instanceFromDataMessage) {
	redisReply** values = new redisReply*[3];
	values[0] = getStringReply("message");
	values[1] = getStringReply(TEST_CHANNEL);
	values[2] = getStringReply(TEST_MESSAGE);
	ChannelMessage message = ChannelMessage::from(getArrayResult(3, values));
	ASSERT_EQ(message.getType(), channelMessage::DATA);
	ASSERT_STREQ(message.getChannelName().c_str(),TEST_CHANNEL);
	ASSERT_STREQ(message.getMessage().c_str(),TEST_MESSAGE);
}

TEST(ChannelMessageTest, instanceFromSubscribeMessage) {
	redisReply** values = new redisReply*[3];
	values[0] = getStringReply("subscribe");
	values[1] = getStringReply(TEST_CHANNEL);
	values[2] = getIntReply(1);
	ChannelMessage message = ChannelMessage::from(getArrayResult(3, values));
	ASSERT_EQ(message.getType(), channelMessage::SUBSCRIBE);
	ASSERT_STREQ(message.getChannelName().c_str(),TEST_CHANNEL);
	ASSERT_STREQ(message.getMessage().c_str(),"1");
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
