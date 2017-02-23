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
static const char * INSTANCES_UTIL_NAMESPACE = "SCALABLE_APPS";

class RedisConnection;

/**
 * A utility class to provide primitive methods for implementing scalable application instances
 */
class InstancesUtil {
private:
	InstancesUtil();
	~InstancesUtil();
public:
	static int reserveInstanceId(RedisConnection& conn, const char* appId);

};



#endif /* INCLUDE_INSTANCESUTIL_HPP_ */
