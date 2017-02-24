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

/**
 * get a new ID for a newly deploying instance of the application
 * (any restart correspond to a new instance)
 */
int InstancesUtil::getNewInstanceId(RedisConnection& conn, const char* appId) {
	std::string command = std::string("INCR ") + INSTANCES_UTIL_NAMESPACE + ":" + appId + ":NODES";
	RedisResult res = conn.cmd(command.c_str());
	if (res.resultType() == INTEGER) {
		return res.intResult();
	}
	return -1;
}

/**
 * first write node's address details as a hash set, with following key schema
 *   <NAMESPACE PREFIX>:<App ID>:INSTANCE:<Node ID>:ADDRESS (TTL set to specified value)
 * then add this node's ID to set of active nodes, with following key schema
 *    <NAMESPACE PREFIX>:<App ID>:INSTANCES
 * then publish this node's ID to pub/sub channel to announce to all active nodes
 *    <NAMESPACE PREFIX>:<App ID>:CHANNELS:INSTANCES
 */
int InstancesUtil::publishNodeDetails(RedisConnection& conn, const char* appId, const int nodeId,
									const char* host, int port, int ttl) {
	if (!conn.isConnected()) {
		return -1;
	}
	std::string key1 = std::string(INSTANCES_UTIL_NAMESPACE) + ":" + appId
			+ ":INSTANCE:" + std::to_string(nodeId) + ":ADDRESS";
	std::string command1 = std::string("HMSET ") + key1
							+ " HOST " + host
							+ " PORT " + std::to_string(port);
	RedisResult res = conn.cmd(command1.c_str());
	if(res.resultType() == ERROR || res.resultType() == FAILED) {
		return -1;
	} else {
		command1 = std::string("EXPIRE ") + key1 + " " + std::to_string(ttl);
		conn.cmd(command1.c_str());
	}

	std::string key2 = std::string(INSTANCES_UTIL_NAMESPACE)
			+ ":" + appId + ":INSTANCES";
	std::string command2 = std::string("SADD ") + key2
			+ " " + std::to_string(nodeId);
	res = conn.cmd(command2.c_str());
	if(res.resultType() != INTEGER || res.intResult() == 0) {
		// roll back if possible
		if (res.resultType() != FAILED) {
			command1 = std::string("DEL ") + key1;
			conn.cmd(command1.c_str());
		}
		return -1;
	}

	std::string command3 = std::string("PUBLISH ") + INSTANCES_UTIL_NAMESPACE
			+ ":" + appId + ":CHANNELS:INSTANCES " + std::to_string(nodeId);
	res = conn.cmd(command3.c_str());
	if(res.resultType() != INTEGER) {
		// roll back if possible
		if (res.resultType() != FAILED) {
			command1 = std::string("DEL ") + key1;
			conn.cmd(command1.c_str());
			command2 = std::string("SREM ") + key2 + " " + std::to_string(nodeId);
			conn.cmd(command2.c_str());
		}
		return -1;
	}
	// return the number of active instances notified
	return res.intResult();
}
