/*
 * InstancesUtil.hpp
 *
 *  Created on: Feb 23, 2017
 *      Author: bhadoria
 */

#ifndef INCLUDE_INSTANCESUTIL_HPP_
#define INCLUDE_INSTANCESUTIL_HPP_

#include<string>
class RedisConnection;
//typedef void (*callbackFunc)(const char*);

/**
 * A utility class to provide primitive methods for implementing scalable application instances
 */
class InstancesUtil {
private:
	InstancesUtil();
	~InstancesUtil();
public:
	// increment and get a counter
	static int incrCounter(RedisConnection& conn, const char* appId, const char* counter);

	// decrement and get a counter
	static int decrCounter(RedisConnection& conn, const char* appId, const char* counter);

	// get a new ID for a newly deploying instance of the application
	static int getNewInstanceId(RedisConnection& conn, const char* appId);

	// register a callback method for any new instance notification
	static int registerInstanceUpCallback(RedisConnection& conn, const char* appId, void (*func)(const char*));

	// register a callback method for any instance down notification
	static int registerInstanceDownCallback(RedisConnection& conn, const char* appId, void (*func)(const char*));

	// publish a node's address details
	static int publishNodeDetails(RedisConnection& conn, const char* appId,
					const int nodeId, const char* host, int port, int ttl);

	// publish a node's address details
	static int getNodeDetails(RedisConnection& conn, const char* appId,
					const int nodeId, std::string& host, int& port);

	// remove a node's details from system
	static int removeNodeDetails(RedisConnection& conn, const char* appId,
					const int nodeId);

	// get a fast lock on system
	static int getFastLock(RedisConnection& conn, const char* appId, const char* lockName, int ttl);

	// release the fast lock
	static int releaseFastLock(RedisConnection& conn, const char* appId, const char* lockName);
};



#endif /* INCLUDE_INSTANCESUTIL_HPP_ */
