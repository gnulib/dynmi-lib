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

int InstancesUtil::reserveInstanceId(RedisConnection& conn, const char* appId) {
	std::string key = std::string("INCR ") + INSTANCES_UTIL_NAMESPACE + ":" + appId + ":CURR_INSTANCE";
	RedisResult res = conn.cmd(key.c_str());
	if (res.resultType() == INTEGER) {
		return res.intResult();
	}
	return -1;
}
