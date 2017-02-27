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
private:
	BroadcastUtil(){stop = true; worker = NULL; workerConn = NULL;}
	~BroadcastUtil();

public:
	static bool isInitialized() { return initialized;}
	// we'll inject the RedisConnection instance to be used by worker,
	// for testability purpose
	static bool initialize(const char * appId, const char* nodeId, RedisConnection * workerConn);
	static bool initializeId(const char * appId, int nodeId, RedisConnection * workerConn);
	static void stopAll(RedisConnection& conn);
	static bool isRunning();
	static int publish(RedisConnection& conn, const char* channelName, const char* message);
	static int addSubscription(RedisConnection& conn, const char* channelName, callbackFunc);
	static int removeSubscription(RedisConnection& conn, const char* channelName);

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
