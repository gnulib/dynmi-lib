/*
 * InstancesUtil.cpp
 *
 *  Created on: Feb 23, 2017
 *      Author: bhadoria
 */


#include "InstancesUtil.hpp"
#include "RedisConnection.hpp"
#include "RedisResult.hpp"

int InstancesUtil::reserveInstanceId(RedisConnection& conn, const char* appId) {
	RedisResult res = RedisResult();
	if (conn.isConnected() && conn.cmd("INCR", res) == 0 && res.resultType() == INTEGER) {
		return res.intResult();
	}
	return -1;
}
