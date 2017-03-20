/*
 * CdMQPayloadTest.cpp
 *
 *  Created on: Mar 19, 2017
 *      Author: bhadoria
 */

#include "CdMQPayload.hpp"
#include "gtest/gtest.h"

using namespace ::testing;

TEST(CdMQPayloadTest, fromJson) {
	CdMQPayload uut = CdMQPayload::fromJson("{\"tag\":\"123\",\"message\":\"test message\"}");
	ASSERT_TRUE(uut.isValid());
    ASSERT_STREQ(uut.getTag().c_str(), "123");
    ASSERT_STREQ(uut.getMessage().c_str(), "test message");
}

TEST(CdMQPayloadTest, fromJsonComplexObject) {
	CdMQPayload uut = CdMQPayload::fromJson("{\"tag\" : \"123\",\n\"message\" : \"{\"f1\" : \"fff\", \"f2\" : \"aaaa\"}\"}");
	ASSERT_TRUE(uut.isValid());
    ASSERT_STREQ(uut.getTag().c_str(), "123");
    ASSERT_STREQ(uut.getMessage().c_str(), "{\"f1\" : \"fff\", \"f2\" : \"aaaa\"}");
}

TEST(CdMQPayloadTest, fromJsonNoStart) {
	CdMQPayload uut = CdMQPayload::fromJson("\"tag\":\"123\",\"message\":\"test message\"}");
	ASSERT_FALSE(uut.isValid());
    ASSERT_STREQ(uut.getTag().c_str(), "");
    ASSERT_STREQ(uut.getMessage().c_str(), "");
}

TEST(CdMQPayloadTest, toJson) {
	CdMQPayload uut = CdMQPayload("123", "test message");
    ASSERT_STREQ(uut.getTag().c_str(), "123");
    ASSERT_STREQ(uut.toJson().c_str(), "{\"tag\":\"123\",\"message\":\"test message\"}");
}

TEST(CdMQPayloadTest, toJsonComplexObject) {
	CdMQPayload uut = CdMQPayload("123", "{\"f1\" : \"fff\", \"f2\" : \"aaaa\"}");
    ASSERT_STREQ(uut.getTag().c_str(), "123");
    ASSERT_STREQ(uut.toJson().c_str(), "{\"tag\":\"123\",\"message\":\"{\"f1\" : \"fff\", \"f2\" : \"aaaa\"}\"}");
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
