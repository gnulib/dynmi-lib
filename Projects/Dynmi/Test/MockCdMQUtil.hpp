/*
 * MockCdMQUtil.hpp
 *
 *  Created on: Mar 21, 2017
 *      Author: bhadoria
 */

#ifndef PROJECTS_DYNMI_TEST_MOCKCDMQUTIL_HPP_
#define PROJECTS_DYNMI_TEST_MOCKCDMQUTIL_HPP_

#include "Dynmi/CdMQUtil.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

class MockCdMQUtil : public CdMQUtil {
public:
	// message will unlock the queue when its discarded
	MOCK_METHOD1(unlock, bool (CdMQMessage& message));

	// add a message at the back of the named queue
	MOCK_METHOD4(enQueue, bool (const std::string& appId, const std::string qName, const std::string& message, const std::string& tag));

	// get a message from front of the named queue
	MOCK_METHOD2(deQueue, CdMQMessage (const std::string qName, int ttl));

	// register callback method to process message when available on named queue
	MOCK_METHOD3(registerReadyCallback, bool (const std::string& appId, const std::string qName, CdMQUtil::callbackFunc));
};


#endif /* PROJECTS_DYNMI_TEST_MOCKCDMQUTIL_HPP_ */
