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
	MOCK_METHOD3(incrCounter, int (RedisConnection& conn, const char* appId, const char* counter));

	// decrement and get a counter
	MOCK_METHOD3(decrCounter, int (RedisConnection& conn, const char* appId, const char* counter));

	// get a new ID for a newly deploying instance of the application
	MOCK_METHOD2(getNewInstanceId, int (RedisConnection& conn, const char* appId));

	// register a callback method for any new instance notification
	MOCK_METHOD3(registerInstanceUpCallback, int (RedisConnection& conn, const char* appId, void (*func)(const char*)));

	// register a callback method for any instance down notification
	MOCK_METHOD3(registerInstanceDownCallback, int (RedisConnection& conn, const char* appId, void (*func)(const char*)));

	// refresh node's address details
	MOCK_METHOD4(refreshNodeDetails, int (RedisConnection& conn, const char* appId, const int nodeId, int ttl));

	// publish a node's address details
	MOCK_METHOD6(publishNodeDetails, int (RedisConnection& conn, const char* appId,
					const int nodeId, const char* host, int port, int ttl));

	// get all active nodes for specified appID
	MOCK_METHOD2(getAllNodes, std::set<std::string> (RedisConnection& conn, const char* appId));

	// get a node's address details
	MOCK_METHOD5(getNodeDetails, int (RedisConnection& conn, const char* appId,
					const int nodeId, std::string& host, int& port));

	// remove a node's details from system
	MOCK_METHOD3(removeNodeDetails, int (RedisConnection& conn, const char* appId,
					const int nodeId));

	// get a fast lock on system
	MOCK_METHOD4(getFastLock, int(RedisConnection& conn, const char* appId, const char* lockName, int ttl));

	// release the fast lock
	MOCK_METHOD3(releaseFastLock, int (RedisConnection& conn, const char* appId, const char* lockName));
};

#endif /* PROJECTS_DYNMI_TEST_MOCKINSTANCESUTIL_HPP_ */
