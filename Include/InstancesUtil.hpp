/*
 * InstancesUtil.hpp
 *
 *  Created on: Feb 23, 2017
 *      Author: bhadoria
 */

#ifndef INCLUDE_INSTANCESUTIL_HPP_
#define INCLUDE_INSTANCESUTIL_HPP_

#include<string>
#include <set>
#include <pthread.h>

class RedisConnection;
//typedef void (*callbackFunc)(const char*);

/**
 * A utility class to provide primitive methods for implementing scalable application instances
 */
class InstancesUtil {
protected:
	InstancesUtil(){};
	virtual ~InstancesUtil();
public:
	// initialize the utility
	static bool initialize(const std::string& redisHost, const int redisPort);

	// UT initialization from mock instance
	static bool initialize(InstancesUtil* mock);

	// get reference to an instance
	static InstancesUtil& instance();

	// increment and get a counter
	int incrCounter(RedisConnection& conn, const char* appId, const char* counter);

	// decrement and get a counter
	int decrCounter(RedisConnection& conn, const char* appId, const char* counter);

	// get a new ID for a newly deploying instance of the application
	int getNewInstanceId(RedisConnection& conn, const char* appId);

	// register a callback method for any new instance notification
	int registerInstanceUpCallback(RedisConnection& conn, const char* appId, void (*func)(const char*));

	// register a callback method for any instance down notification
	int registerInstanceDownCallback(RedisConnection& conn, const char* appId, void (*func)(const char*));

	// refresh node's address details
	int refreshNodeDetails(RedisConnection& conn, const char* appId,
					const int nodeId, int ttl);

	// publish a node's address details
	int publishNodeDetails(RedisConnection& conn, const char* appId,
					const int nodeId, const char* host, int port, int ttl);

	// get all active nodes for specified appID
	std::set<std::string> getAllNodes(RedisConnection& conn, const char* appId);

	// get a node's address details
	int getNodeDetails(RedisConnection& conn, const char* appId,
					const int nodeId, std::string& host, int& port);

	// remove a node's details from system
	int removeNodeDetails(RedisConnection& conn, const char* appId,
					const int nodeId);

	// get a fast lock on system
	int getFastLock(RedisConnection& conn, const char* appId, const char* lockName, int ttl);

	// release the fast lock
	int releaseFastLock(RedisConnection& conn, const char* appId, const char* lockName);

private:
	static InstancesUtil* inst;
	static bool initialized;
	static pthread_mutex_t mtx;
};



#endif /* INCLUDE_INSTANCESUTIL_HPP_ */
