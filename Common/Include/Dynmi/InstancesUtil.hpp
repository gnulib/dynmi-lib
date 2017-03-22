/*
 * InstancesUtil.hpp
 *
 *  Created on: Feb 23, 2017
 *      Author: bhadoria
 */

#ifndef INCLUDE_INSTANCESUTIL_HPP_
#define INCLUDE_INSTANCESUTIL_HPP_

#include<string>
#include <map>
#include <set>
#include <pthread.h>
#include "Dynmi/BroadcastUtil.hpp"

/**
 * A utility class to provide primitive methods for implementing scalable application instances
 */
class InstancesUtil {
protected:
	InstancesUtil(){};
	virtual ~InstancesUtil();

	// my callback method for listening to all instance up/down notifications
	static void myCallbackFunc(const char*, const char*);
public:
	// initialize the utility
	static bool initialize(const std::string& redisHost, const int redisPort);

	// UT initialization from mock instance
	static bool initialize(InstancesUtil* mock);

	// get reference to an instance
	static InstancesUtil& instance();

	// increment and get a counter
	virtual int incrCounter(const char* appId, const char* counter);

	// decrement and get a counter
	virtual int decrCounter(const char* appId, const char* counter);

	// get a new ID for a newly deploying instance of the application
	virtual int getNewInstanceId(const char* appId);

	// register a callback method for any new instance notification
	virtual int registerInstanceUpCallback(const char* appId, BroadcastUtil::callbackFunc);

	// register a callback method for any instance down notification
	virtual int registerInstanceDownCallback(const char* appId, BroadcastUtil::callbackFunc);

	// refresh node's address details
	virtual int refreshNodeDetails(const char* appId, const int nodeId, int ttl);

	// publish a node's address details
	virtual int publishNodeDetails(const char* appId, const int nodeId, const char* host, int port, int ttl);

	// get all active nodes for specified appID
	virtual std::set<std::string> getAllNodes(const char* appId);

	// get a node's address details
	virtual int getNodeDetails(const char* appId, const int nodeId, std::string& host, int& port);

	// remove a node's details from system
	virtual int removeNodeDetails(const char* appId, const int nodeId);

	// get a fast lock on system
	virtual int getFastLock(const char* appId, const char* lockName, int ttl);

	// release the fast lock
	virtual int releaseFastLock(const char* appId, const char* lockName);

private:
	static InstancesUtil* inst;
	static bool initialized;
	static bool isTest;
	static pthread_mutex_t mtx;
	std::map<std::string,std::set<BroadcastUtil::callbackFunc> > myCallbacks;
};



#endif /* INCLUDE_INSTANCESUTIL_HPP_ */
