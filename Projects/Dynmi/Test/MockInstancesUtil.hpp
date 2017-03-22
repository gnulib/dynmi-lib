/*
 * MockInstancesUtil.hpp
 *
 *  Created on: Mar 20, 2017
 *      Author: bhadoria
 */

#ifndef PROJECTS_DYNMI_TEST_MOCKINSTANCESUTIL_HPP_
#define PROJECTS_DYNMI_TEST_MOCKINSTANCESUTIL_HPP_

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "Dynmi/InstancesUtil.hpp"


class MockInstancesUtil : public InstancesUtil {
public:
	// increment and get a counter
	MOCK_METHOD2(incrCounter, int (const char* appId, const char* counter));

	// decrement and get a counter
	MOCK_METHOD2(decrCounter, int (const char* appId, const char* counter));

	// get a new ID for a newly deploying instance of the application
	MOCK_METHOD1(getNewInstanceId, int (const char* appId));

	// register a callback method for any new instance notification
	MOCK_METHOD2(registerInstanceUpCallback, int (const char* appId, BroadcastUtil::callbackFunc));

	// register a callback method for any instance down notification
	MOCK_METHOD2(registerInstanceDownCallback, int (const char* appId, BroadcastUtil::callbackFunc));

	// refresh node's address details
	MOCK_METHOD3(refreshNodeDetails, int (const char* appId, const int nodeId, int ttl));

	// publish a node's address details
	MOCK_METHOD5(publishNodeDetails, int (const char* appId,
					const int nodeId, const char* host, int port, int ttl));

	// get all active nodes for specified appID
	MOCK_METHOD1(getAllNodes, std::set<std::string> (const char* appId));

	// get a node's address details
	MOCK_METHOD4(getNodeDetails, int (const char* appId,
					const int nodeId, std::string& host, int& port));

	// remove a node's details from system
	MOCK_METHOD2(removeNodeDetails, int (const char* appId,
					const int nodeId));

	// get a fast lock on system
	MOCK_METHOD3(getFastLock, int(const char* appId, const char* lockName, int ttl));

	// release the fast lock
	MOCK_METHOD2(releaseFastLock, int (const char* appId, const char* lockName));
};

#endif /* PROJECTS_DYNMI_TEST_MOCKINSTANCESUTIL_HPP_ */
