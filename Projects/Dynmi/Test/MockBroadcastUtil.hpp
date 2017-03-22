/*
 * MockBroadcastUtil.hpp
 *
 *  Created on: Mar 20, 2017
 *      Author: bhadoria
 */

#ifndef PROJECTS_DYNMI_TEST_MOCKBROADCASTUTIL_HPP_
#define PROJECTS_DYNMI_TEST_MOCKBROADCASTUTIL_HPP_

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "Dynmi/BroadcastUtil.hpp"

class MockBroadcastUtil : public BroadcastUtil {
public:
	// get initialization status
	MOCK_METHOD0(isInitialized, bool ());

	// publish a message to broadcast on named channel
	MOCK_METHOD2(publish, int (const char* channelName, const char* message));

	// subscribe this instance to receive messages published on named channel
	MOCK_METHOD2(addSubscription, int (const char* channelName, BroadcastUtil::callbackFunc));

	// remove subscription of this instance from named channel
	MOCK_METHOD1(removeSubscription, int (const char* channelName));
};


#endif /* PROJECTS_DYNMI_TEST_MOCKBROADCASTUTIL_HPP_ */
