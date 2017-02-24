/*
 * InstancesUtil.hpp
 *
 *  Created on: Feb 23, 2017
 *      Author: bhadoria
 */

#ifndef INCLUDE_INSTANCESUTIL_HPP_
#define INCLUDE_INSTANCESUTIL_HPP_

// a namespace prefix to use with all our keys with Redis
// change this value to use different/custom app namespace
static const char * INSTANCES_UTIL_NAMESPACE = "SCALABLE_APP";

class RedisConnection;

/**
 * A utility class to provide primitive methods for implementing scalable application instances
 */
class InstancesUtil {
private:
	InstancesUtil();
	~InstancesUtil();
public:
	// get a new ID for a newly deploying instance of the application
	static int getNewInstanceId(RedisConnection& conn, const char* appId);

	// publish a node's address details
	static int publishNodeDetails(RedisConnection& conn, const char* appId,
					const int nodeId, const char* host, int port, int ttl);
};



#endif /* INCLUDE_INSTANCESUTIL_HPP_ */
