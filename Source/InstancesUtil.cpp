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
#include <ctime>

/**
 * increment a counter and get value
 */
int InstancesUtil::incrCounter(RedisConnection& conn, const char* appId, const char* counter) {
	if (!conn.isConnected()) {
		return -1;
	}

	std::string key = std::string(INSTANCES_UTIL_NAMESPACE)
			+ ":" + appId + ":COUNTERS:" + counter;

	std::string command = std::string("INCR ") + key;
	RedisResult res = conn.cmd(command.c_str());
	if(res.resultType() != INTEGER) {
		return -1;
	}

	return res.intResult();
}

/**
 * decrement a counter and get value
 */
int InstancesUtil::decrCounter(RedisConnection& conn, const char* appId, const char* counter) {
	if (!conn.isConnected()) {
		return -1;
	}

	std::string key = std::string(INSTANCES_UTIL_NAMESPACE)
			+ ":" + appId + ":COUNTERS:" + counter;

	std::string command = std::string("DECR ") + key;
	RedisResult res = conn.cmd(command.c_str());
	if(res.resultType() != INTEGER) {
		return -1;
	}

	return res.intResult();
}

/**
 * get a new ID for a newly deploying instance of the application
 * (any restart correspond to a new instance)
 */
int InstancesUtil::getNewInstanceId(RedisConnection& conn, const char* appId) {
	return InstancesUtil::incrCounter(conn, appId, "NODES");
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
			+ ":" + appId + ":CHANNELS:INSTANCE_UP " + std::to_string(nodeId);
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

/**
 * remove a node's entries from system, and broadcast node down
 */
int InstancesUtil::removeNodeDetails(RedisConnection& conn, const char* appId,
					const int nodeId) {
	if (!conn.isConnected()) {
		return -1;
	}

	std::string command1 = std::string("PUBLISH ") + INSTANCES_UTIL_NAMESPACE
			+ ":" + appId + ":CHANNELS:INSTANCE_DOWN " + std::to_string(nodeId);
	RedisResult res = conn.cmd(command1.c_str());
	if(res.resultType() == ERROR || res.resultType() == FAILED) {
		return -1;
	}

	std::string key2 = std::string(INSTANCES_UTIL_NAMESPACE)
			+ ":" + appId + ":INSTANCES";
	std::string command2 = std::string("SREM ") + key2
			+ " " + std::to_string(nodeId);
	res = conn.cmd(command2.c_str());
	if(res.resultType() == ERROR || res.resultType() == FAILED) {
		return -1;
	}

	std::string key3 = std::string(INSTANCES_UTIL_NAMESPACE) + ":" + appId
			+ ":INSTANCE:" + std::to_string(nodeId) + ":ADDRESS";
	std::string command3 = std::string("DEL ") + key3;
	res = conn.cmd(command3.c_str());
	if(res.resultType() == ERROR || res.resultType() == FAILED) {
		return -1;
	}

	return 0;
}

/**
 * get a fast lock on system as following:
 *	1.	execute "SETNX <mutex key> <current Unix timestamp + mutex timeout + 1>"
 *		on the key for the mutex name
 *	2.	if return value is "1", then proceed with operation, and then delete the key
 *	3.	if return value is "0", then
 *		a.	get mutex key value (timestamp of expiry)
 *		b.	if it is more than current time, then wait (mutex in use)
 *		c.	otherwise try following:
 *			1)	execute "GETSET <mutex key> <current Unix timestamp + mutex timeout + 1>"
 *			2)	check the return value to see if this is same as old expired value
 *			3)	if return value is not same as original expired then some other instance
 *				got the mutex, and this instance needs to wait (or abort)
 *			4)	otherwise (if return value is same as expired), then instance successfully
 *				acquired the mutex, proceed with operation
 */
int InstancesUtil::getFastLock(RedisConnection& conn, const char* appId, const char* lockName, int ttl) {
	if (!conn.isConnected()) {
		return -1;
	}

	std::string key = std::string(INSTANCES_UTIL_NAMESPACE)
			+ ":" + appId + ":LOCKS:" + lockName;
	std::time_t now = std::time(0);

	// STEP 1
	std::string command = std::string("SETNX ") + key + " " + std::to_string(now + ttl + 1);
	RedisResult res = conn.cmd(command.c_str());
	if(res.resultType() == ERROR || res.resultType() == FAILED) {
		return -1;
	}

	// STEP 2
	if (res.intResult() == 1) {
		// set an expiry to this lock
		command = std::string("EXPIRE ") + key + " " + std::to_string(ttl + 1);
		conn.cmd(command.c_str());
		return 0;
	}

	// STEP 3.a
	command = std::string("GET ") + key;
	res = conn.cmd(command.c_str());
	if(res.resultType() != STRING) {
		return -1;
	}

	// STEP 3.b
	long expiry = std::stol(res.strResult());
	if(expiry > now) {
		return expiry - now;
	}

	// STEP 3.c.1
	command = std::string("GETSET ") + key + " " + std::to_string(now + ttl + 1);
	res = conn.cmd(command.c_str());
	if(res.resultType() != STRING) {
		return -1;
	}

	// STEP 3.c.2
	long newExpiry = std::stol(res.strResult());
	if(expiry != newExpiry) {
		// revert back lock's expire value to newExpiry
		command = std::string("SET ") + key + " " + res.strResult();
		conn.cmd(command.c_str());

		// STEP 3.c.3
		return newExpiry - now;
	}

	// STEP 3.c.4
	// set an expiry to this lock
	command = std::string("EXPIRE ") + key + " " + std::to_string(ttl + 1);
	conn.cmd(command.c_str());
	return 0;
}

int InstancesUtil::releaseFastLock(RedisConnection& conn, const char* appId, const char* lockName) {
	if (!conn.isConnected()) {
		return -1;
	}

	std::string key = std::string(INSTANCES_UTIL_NAMESPACE)
			+ ":" + appId + ":LOCKS:" + lockName;

	std::string command = std::string("DEL ") + key;
	RedisResult res = conn.cmd(command.c_str());
	if(res.resultType() == ERROR || res.resultType() == FAILED) {
		return -1;
	}

	return 0;
}
