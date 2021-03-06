/*
 * BroadcastUtil.hpp
 *
 *  Created on: Feb 24, 2017
 *      Author: bhadoria
 */

#ifndef INCLUDE_BROADCASTUTIL_HPP_
#define INCLUDE_BROADCASTUTIL_HPP_

#include <string>
#include <pthread.h>
#include <map>
#include <set>

static const std::string CONTROL = ":CHANNELS:CONTROL:";
static const std::string MESSAGE = ":CHANNELS:MESSAGE:";
static const std::string SUBSCRIBE = "SUBSCRIBE ";
static const std::string UNSUBSCRIBE = "UNSUBSCRIBE ";
static std::string COMMAND_DELIM = "\"";
static std::string ADD_COMMAND = "ADD_CHANNEL ";
static std::string REMOVE_COMMAND = "REMOVE_CHANNEL ";
static std::string STOP_COMMAND = "STOP";

class BroadcastUtil {
protected:
	BroadcastUtil(){stop = true; worker = NULL;}
	virtual ~BroadcastUtil();

public:
	// get reference to initialized instance
	static BroadcastUtil& instance();

	// initialize the library before it can be used
	static bool initialize(const char * appId, const char* nodeId);

	// another way to initialize the library where instance's node ID is used as uniqueId
	static bool initializeWithId(const char * appId, int nodeId);

	// a UT compatible initialization method
	static bool initialize(BroadcastUtil* mock);

	// get initialization status
	virtual bool isInitialized() { return initialized;}

	// stop all subscriptions and worker thread for this channel, used during shutdown
	static void stopAll();

	// check if worker thread is running
	static bool isRunning();

	// publish a message to broadcast on named channel
	virtual int publish(const char* channelName, const char* message);

	typedef void (*callbackFunc)(const char* channel, const char* notification);
	// subscribe this instance to receive messages published on named channel
	virtual int addSubscription(const char* channelName, BroadcastUtil::callbackFunc);

	// remove subscription of this instance from named channel
	virtual int removeSubscription(const char* channelName);

protected:
	std::string getControlChannel();

private:
	static void* workerThread(void *);
	static void notifyCallbacks(std::string channel, std::string payload);

private:
	static BroadcastUtil* inst;
	static bool initialized;
	static bool isTest;
	static pthread_mutex_t mtx;
	pthread_t* worker;
	std::string appId;
	std::string nodeId;
	std::string controlChannel;
	bool stop;
	std::map<std::string,std::set<callbackFunc> > myCallbacks;
};


#endif /* INCLUDE_BROADCASTUTIL_HPP_ */
