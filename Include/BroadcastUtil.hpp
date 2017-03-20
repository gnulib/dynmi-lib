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
class RedisConnection;

static const std::string CONTROL = ":CHANNELS:CONTROL:";
static const std::string MESSAGE = ":CHANNELS:MESSAGE:";
static const std::string SUBSCRIBE = "SUBSCRIBE ";
static const std::string UNSUBSCRIBE = "UNSUBSCRIBE ";
static std::string COMMAND_DELIM = "\"";
static std::string ADD_COMMAND = "ADD_CHANNEL ";
static std::string REMOVE_COMMAND = "REMOVE_CHANNEL ";
static std::string STOP_COMMAND = "STOP";
typedef void (*callbackFunc)(const char*);

class BroadcastUtil {
protected:
	BroadcastUtil(){stop = true; worker = NULL; workerConn = NULL;}
	virtual ~BroadcastUtil();

public:
	// get reference to initialized instance
	static BroadcastUtil& instance();

	// initialize the library before it can be used. we inject the RedisConnection instance
	// to be used by background worker thread for testability purpose
	static bool initialize(const char * appId, const char* uniqueId, RedisConnection * workerConn);

	// another way to initialize the library where instance's node ID is used as uniqueId
	static bool initializeWithId(const char * appId, int nodeId, RedisConnection * workerConn);

	// a UT compatible initialization method
	static bool initialize(BroadcastUtil* mock);

	// get initialization status
	bool isInitialized() { return initialized;}

	// stop all subscriptions and worker thread for this channel, used during shutdown
	static void stopAll(RedisConnection& conn);

	// check if worker thread is running
	static bool isRunning();

	// publish a message to broadcast on named channel
	int publish(RedisConnection& conn, const char* channelName, const char* message);

	// subscribe this instance to receive messages published on named channel
	int addSubscription(RedisConnection& conn, const char* channelName, callbackFunc);

	// remove subscription of this instance from named channel
	int removeSubscription(RedisConnection& conn, const char* channelName);

protected:
	std::string getControlChannel();

private:
	static void* workerThread(void *);
	static void notifyCallbacks(std::string channel, std::string payload);

private:
	static BroadcastUtil* inst;
	static bool initialized;
	static pthread_mutex_t mtx;
	pthread_t* worker;
	RedisConnection* workerConn;
	std::string appId;
	std::string suffix;
	std::string controlChannel;
	bool stop;
	std::map<std::string,std::set<callbackFunc> > myCallbacks;
};


#endif /* INCLUDE_BROADCASTUTIL_HPP_ */
