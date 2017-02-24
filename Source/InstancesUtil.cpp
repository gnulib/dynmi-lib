/*
 * InstancesUtil.cpp
 *
 *  Created on: Feb 23, 2017
 *      Author: bhadoria
 */


#include "InstancesUtil.hpp"
#include "RedisConnection.hpp"
#include "RedisResult.hpp"
#include <string>

int InstancesUtil::getNewInstanceId(RedisConnection& conn, const char* appId) {
	std::string key = std::string("INCR ") + INSTANCES_UTIL_NAMESPACE + ":" + appId + ":INSTANCES";
	RedisResult res = conn.cmd(key.c_str());
	if (res.resultType() == INTEGER) {
		return res.intResult();
	}
	return -1;
}
