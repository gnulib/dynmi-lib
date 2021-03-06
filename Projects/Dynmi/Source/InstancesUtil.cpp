/*
 * InstancesUtil.cpp
 *
 *  Created on: Feb 23, 2017
 *      Author: bhadoria
 */

#include "Dynmi/DynmiGlobals.hpp"
#include "Dynmi/InstancesUtil.hpp"
#include "Dynmi/BroadcastUtil.hpp"
#include "Dynmi/RedisConnection.hpp"
#include "Dynmi/RedisConnectionTL.hpp"
#include "Dynmi/RedisResult.hpp"
#include <string>
#include <ctime>
#include <sstream>

InstancesUtil* InstancesUtil::inst = NULL;
bool InstancesUtil::initialized = false;
bool InstancesUtil::isTest = false;
pthread_mutex_t InstancesUtil::mtx = PTHREAD_MUTEX_INITIALIZER;

InstancesUtil::~InstancesUtil() {
	if (inst == this) initialized = false;
}

bool InstancesUtil::initialize(const std::string& redisHost, const int redisPort) {
	if (!initialized) {
		pthread_mutex_lock(&InstancesUtil::mtx);
		try {
			// check once more, after we get lock, in case someone else had already initialized
			// while we were waiting
			if (!initialized) {
				InstancesUtil::inst = new InstancesUtil();
				RedisConnectionTL::initialize(redisHost, redisPort);
				initialized = true;
			}
		} catch (...) {
			// failed to initialize
			initialized = false;
			if (InstancesUtil::inst)
				delete InstancesUtil::inst;
			InstancesUtil::inst = NULL;
		}
		pthread_mutex_unlock(&InstancesUtil::mtx);
	}
	return initialized;
}

bool InstancesUtil::initialize(InstancesUtil* mock) {
	isTest = true;
	inst = mock;
	initialized = false;
	return true;
}

InstancesUtil& InstancesUtil::instance() {
	if (InstancesUtil::isTest) return *inst;
	static InstancesUtil mock = InstancesUtil();
	if (!initialized) {
		// return uninitialized instance and hope for best
		return mock;
	}
	return *inst;
}

/**
 * increment a counter and get value
 */
int InstancesUtil::incrCounter(const char* appId, const char* counter) {
	RedisConnection& conn = RedisConnectionTL::instance();
	if (!conn.isConnected()) {
		return -1;
	}

	std::string key = std::string(NAMESPACE_PREFIX)
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
int InstancesUtil::decrCounter(const char* appId, const char* counter) {
	RedisConnection& conn = RedisConnectionTL::instance();
	if (!conn.isConnected()) {
		return -1;
	}

	std::string key = std::string(NAMESPACE_PREFIX)
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
int InstancesUtil::getNewInstanceId(const char* appId) {
	return InstancesUtil::incrCounter(appId, "NODES");
}

std::string channelInstanceUp(const char * appId) {
	return std::string(NAMESPACE_PREFIX)
	+ ":" + appId + ":CHANNELS:INSTANCE_UP";
}

std::string channelInstanceDown(const char * appId) {
	return std::string(NAMESPACE_PREFIX)
	+ ":" + appId + ":CHANNELS:INSTANCE_DOWN";
}

/**
 * my own call back handler to marshal notifications before invoking registered callbacks
 */
void InstancesUtil::myCallbackFunc(const char* channel, const char* nodeId) {
	std::set<BroadcastUtil::callbackFunc> callbacks = inst->myCallbacks[channel];
	std::string appId = channel;
	size_t end = appId.find(":CHANNELS:INSTANCE_");
	if (end == std::string::npos) return;
	size_t start = appId.find(":", NAMESPACE_PREFIX.length());
	if (start == std::string::npos) return;
	start++;
	if (start >= end) return;
	appId = appId.substr(start, end-start);
	for (std::set<BroadcastUtil::callbackFunc>::iterator callback = callbacks.begin();
			callback != callbacks.end(); ++callback) {
		(*callback)(appId.c_str(), nodeId);
	}
}

/**
 * register a callback method to be notified whenever a new instance for this application comes up
 */
int InstancesUtil::registerInstanceUpCallback(const char* appId, BroadcastUtil::callbackFunc func) {
	if (!BroadcastUtil::instance().isInitialized()) return -1;
	myCallbacks[channelInstanceUp(appId)].insert(func);
	return BroadcastUtil::instance().addSubscription(channelInstanceUp(appId).c_str(), myCallbackFunc);
}

/**
 * register a callback method to be notified whenever a instance for this application goes down
 */
int InstancesUtil::registerInstanceDownCallback(const char* appId, BroadcastUtil::callbackFunc func) {
	if (!BroadcastUtil::instance().isInitialized()) return -1;
	myCallbacks[channelInstanceDown(appId)].insert(func);
	return BroadcastUtil::instance().addSubscription(channelInstanceDown(appId).c_str(), myCallbackFunc);
}

int InstancesUtil::refreshNodeDetails(const char* appId, const int nodeId, int ttl) {
	RedisConnection& conn = RedisConnectionTL::instance();
	if (!conn.isConnected()) {
		return -1;
	}
#if __cplusplus >= 201103L
	std::string nodeIdStr = std::to_string(nodeId);
	std::string ttlStr = std::to_string(ttl);
#else
	std::string nodeIdStr = static_cast<std::ostringstream*>( &(std::ostringstream() << (nodeId)) )->str();
	std::string ttlStr = static_cast<std::ostringstream*>( &(std::ostringstream() << (ttl)) )->str();
#endif
	std::string key1 = std::string(NAMESPACE_PREFIX) + ":" + appId
			+ ":INSTANCE:" + nodeIdStr + ":ADDRESS";
	std::string command1 = std::string("EXPIRE ") + key1 + " " + ttlStr;
	return conn.cmd(command1.c_str()).intResult();
}

/**
 * first write node's address details as a hash set, with following key schema
 *   <NAMESPACE PREFIX>:<App ID>:INSTANCE:<Node ID>:ADDRESS (TTL set to specified value)
 * then add this node's ID to set of active nodes, with following key schema
 *    <NAMESPACE PREFIX>:<App ID>:INSTANCES
 * then publish this node's ID to pub/sub channel to announce to all active nodes
 *    <NAMESPACE PREFIX>:<App ID>:CHANNELS:INSTANCES
 */
int InstancesUtil::publishNodeDetails(const char* appId, const int nodeId,
									const char* host, int port, int ttl) {
	RedisConnection& conn = RedisConnectionTL::instance();
	if (!conn.isConnected()) {
		return -1;
	}
#if __cplusplus >= 201103L
	std::string nodeIdStr = std::to_string(nodeId);
	std::string portStr = std::to_string(port);
	std::string ttlStr = std::to_string(ttl);
#else
	std::string nodeIdStr = static_cast<std::ostringstream*>( &(std::ostringstream() << (nodeId)) )->str();
	std::string portStr = static_cast<std::ostringstream*>( &(std::ostringstream() << (port)) )->str();
	std::string ttlStr = static_cast<std::ostringstream*>( &(std::ostringstream() << (ttl)) )->str();
#endif
	std::string key1 = std::string(NAMESPACE_PREFIX) + ":" + appId
			+ ":INSTANCE:" + nodeIdStr + ":ADDRESS";
	std::string command1 = std::string("HMSET ") + key1
							+ " HOST " + host
							+ " PORT " + portStr;
	RedisResult res = conn.cmd(command1.c_str());
	if(res.resultType() == ERROR || res.resultType() == FAILED) {
		return -1;
	} else {
		command1 = std::string("EXPIRE ") + key1 + " " + ttlStr;
		conn.cmd(command1.c_str());
	}

	std::string key2 = std::string(NAMESPACE_PREFIX)
			+ ":" + appId + ":INSTANCES";
	std::string command2 = std::string("SADD ") + key2
			+ " " + nodeIdStr;
	res = conn.cmd(command2.c_str());
	if(res.resultType() != INTEGER || res.intResult() == 0) {
		// roll back if possible
		if (res.resultType() != FAILED) {
			command1 = std::string("DEL ") + key1;
			conn.cmd(command1.c_str());
		}
		return -1;
	}

	res = conn.publish(channelInstanceUp(appId).c_str(), nodeIdStr.c_str());
	if(res.resultType() != INTEGER) {
		// roll back if possible
		if (res.resultType() != FAILED) {
			command1 = std::string("DEL ") + key1;
			conn.cmd(command1.c_str());
			command2 = std::string("SREM ") + key2 + " " + nodeIdStr;
			conn.cmd(command2.c_str());
		}
		return -1;
	}
	// return the number of active instances notified
	return res.intResult();
}

std::set<std::string> InstancesUtil::getAllNodes(const char* appId){
	std::set<std::string> nodes;
	RedisConnection& conn = RedisConnectionTL::instance();
	if (!conn.isConnected()) {
		return nodes;
	}
	std::string key = std::string(NAMESPACE_PREFIX)
			+ ":" + appId + ":INSTANCES";
	std::string command = std::string("SMEMBERS ") + key;
	RedisResult res = conn.cmd(command.c_str());
	if(res.resultType() != ARRAY) {
		return nodes;
	}
	for (int i=0; i < res.arraySize(); i++) {
		nodes.insert(res.arrayResult(i).strResult());
	}
	return nodes;
}

int InstancesUtil::getNodeDetails(const char* appId,
					const int nodeId, std::string& host, int& port){
	RedisConnection& conn = RedisConnectionTL::instance();
	if (!conn.isConnected()) {
		return -1;
	}
#if __cplusplus >= 201103L
	std::string nodeIdStr = std::to_string(nodeId);
#else
	std::string nodeIdStr = static_cast<std::ostringstream*>( &(std::ostringstream() << (nodeId)) )->str();
#endif
	std::string key1 = std::string(NAMESPACE_PREFIX) + ":" + appId
			+ ":INSTANCE:" + nodeIdStr + ":ADDRESS";
	std::string command1 = std::string("HGETALL ") + key1;
	RedisResult res = conn.cmd(command1.c_str());
	if(res.resultType() != ARRAY || res.arraySize() != 4) {
		return -1;
	}

	// copy result into provided references
	host = res.arrayResult(1).strResult();
#if __cplusplus >= 201103L
	port = std::stol(res.arrayResult(3).strResult());
#else
	std::istringstream(res.arrayResult(3).strResult()) >> port;
#endif


	// return success
	return 0;
}

/**
 * remove a node's entries from system, and broadcast node down
 */
int InstancesUtil::removeNodeDetails(const char* appId, const int nodeId) {
	RedisConnection& conn = RedisConnectionTL::instance();
	if (!conn.isConnected()) {
		return -1;
	}
#if __cplusplus >= 201103L
	std::string nodeIdStr = std::to_string(nodeId);
	std::string portStr = std::to_string(port);
	std::string ttlStr = std::to_string(ttl);
#else
	std::string nodeIdStr = static_cast<std::ostringstream*>( &(std::ostringstream() << (nodeId)) )->str();
#endif

	RedisResult res = conn.publish(channelInstanceDown(appId).c_str(), nodeIdStr.c_str());
	if(res.resultType() == ERROR || res.resultType() == FAILED) {
		return -1;
	}

	std::string key2 = std::string(NAMESPACE_PREFIX)
			+ ":" + appId + ":INSTANCES";
	std::string command2 = std::string("SREM ") + key2
			+ " " + nodeIdStr;
	res = conn.cmd(command2.c_str());
	if(res.resultType() == ERROR || res.resultType() == FAILED) {
		return -1;
	}

	std::string key3 = std::string(NAMESPACE_PREFIX) + ":" + appId
			+ ":INSTANCE:" + nodeIdStr + ":ADDRESS";
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
int InstancesUtil::getFastLock(const char* appId, const char* lockName, int ttl) {
	RedisConnection& conn = RedisConnectionTL::instance();
	if (!conn.isConnected()) {
		return -1;
	}
	std::time_t now = std::time(0);
#if __cplusplus >= 201103L
	std::string ttlStr = std::to_string(ttl);
	std::string expireStr = std::to_string(now + ttlStr + 1);
#else
	std::string ttlStr = static_cast<std::ostringstream*>( &(std::ostringstream() << (ttl + 1)) )->str();
	std::string expireStr = static_cast<std::ostringstream*>( &(std::ostringstream() << (now + ttl + 1)) )->str();
#endif

	std::string key = std::string(NAMESPACE_PREFIX)
			+ ":" + appId + ":LOCKS:" + lockName;

	// STEP 1
	std::string command = std::string("SETNX ") + key + " " + expireStr;
	RedisResult res = conn.cmd(command.c_str());
	if(res.resultType() == ERROR || res.resultType() == FAILED) {
		return -1;
	}

	// STEP 2
	if (res.intResult() == 1) {
		// set an expiry to this lock
		command = std::string("EXPIRE ") + key + " " + ttlStr;
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
#if __cplusplus >= 201103L
	long expiry = std::stol(res.strResult());
#else
	long expiry;
	std::istringstream(res.strResult()) >> expiry;
#endif
	if(expiry > now) {
		return expiry - now;
	}

	// STEP 3.c.1
	command = std::string("GETSET ") + key + " " + expireStr;
	res = conn.cmd(command.c_str());
	if(res.resultType() != STRING) {
		return -1;
	}

	// STEP 3.c.2
#if __cplusplus >= 201103L
	long newExpiry = std::stol(res.strResult());
#else
	long newExpiry;
	std::istringstream(res.strResult()) >> newExpiry;
#endif
	if(expiry != newExpiry) {
		// revert back lock's expire value to newExpiry
		command = std::string("SET ") + key + " " + res.strResult();
		conn.cmd(command.c_str());

		// STEP 3.c.3
		return newExpiry - now;
	}

	// STEP 3.c.4
	// set an expiry to this lock
	command = std::string("EXPIRE ") + key + " " + ttlStr;
	conn.cmd(command.c_str());
	return 0;
}

int InstancesUtil::releaseFastLock(const char* appId, const char* lockName) {
	RedisConnection& conn = RedisConnectionTL::instance();
	if (!conn.isConnected()) {
		return -1;
	}

	std::string key = std::string(NAMESPACE_PREFIX)
			+ ":" + appId + ":LOCKS:" + lockName;

	std::string command = std::string("DEL ") + key;
	RedisResult res = conn.cmd(command.c_str());
	if(res.resultType() == ERROR || res.resultType() == FAILED) {
		return -1;
	}

	return 0;
}
