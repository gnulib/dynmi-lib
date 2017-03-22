/*
 * CdMQMessageTest.cpp
 *
 *  Created on: Mar 16, 2017
 *      Author: bhadoria
 */




#include "Dynmi/CdMQMessage.hpp"
#include "Dynmi/CdMQUtil.hpp"
#include <iostream>
#include <pthread.h>
#include <string>
#include <map>
#include "Dynmi/RedisConnection.hpp"
#include "MockCdMQUtil.hpp"

#include "gtest/gtest.h"
using namespace ::testing;

const std::string TEST_APP_ID = "99";
const std::string TEST_DATA = "{test data}";
const std::string TEST_Q_NAME = "testQ";

//// a mock util class for testing
//class CdMQUtil {
//public:
//	static bool unlock(CdMQMessage& message) {return true;}
//};

// a derived class to test the protected class
class CdMQMessageUUT : public CdMQMessage {
public:
	CdMQMessageUUT(const std::string& data, const std::string& appId, const std::string& qName)
	: CdMQMessage(data, appId, qName){}
};

// test pthreads library
TEST(CdMQMessageTest, testPthreads) {
	std::cout << "running on thread: " << pthread_self() << std::endl;
	std::map<pthread_t, RedisConnection*> pool;
	std::cout << "and thread local pointer is: " << pool[pthread_self()] << std::endl;
	std::cout << "and thread local pointer is: " << pool[pthread_self()] << std::endl;
}

// test that when instance is created it has the lock
TEST(CdMQMessageTest, lockAtCreation) {
	MockCdMQUtil mock;
	CdMQUtil::initialize(&mock);
	EXPECT_CALL(mock, unlock(_))
	.Times(1);

	CdMQMessage* uut = new CdMQMessageUUT(TEST_DATA, TEST_APP_ID, TEST_Q_NAME);
	ASSERT_TRUE(uut->isValid());
	delete uut;
}

// test data container
TEST(CdMQMessageTest, dataContainer) {
	MockCdMQUtil mock;
	CdMQUtil::initialize(&mock);
	EXPECT_CALL(mock, unlock(_))
	.Times(1);

	CdMQMessage* uut = new CdMQMessageUUT(TEST_DATA, TEST_APP_ID, TEST_Q_NAME);
	ASSERT_STREQ(uut->getData().c_str(), TEST_DATA.c_str());
	delete uut;
}

// test assignment operator behavior
TEST(CdMQMessageTest, unlockAfterCopy) {
	MockCdMQUtil mock;
	CdMQUtil::initialize(&mock);
	EXPECT_CALL(mock, unlock(_))
	.Times(1);

	CdMQMessage* uut = new CdMQMessageUUT(TEST_DATA, TEST_APP_ID, TEST_Q_NAME);
	CdMQMessage copy = *uut;
	// original should have lost the lock
	ASSERT_FALSE(uut->isValid());
	// copy should have gained the lock
	ASSERT_TRUE(copy.isValid());
	delete uut;
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
