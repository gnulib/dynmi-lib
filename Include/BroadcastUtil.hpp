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
class RedisConnection;

class BroadcastUtil {
private:
	BroadcastUtil(){stop = true; worker = NULL; workerConn = NULL;}
	~BroadcastUtil();

public:
	static bool isInitialized() { return initialized;}
	// we'll inject the RedisConnection instance to be used by worker,
	// for testability purpose
	static bool initialize(const char * appId, RedisConnection * workerConn);
	static BroadcastUtil& instance(const char* appId);
	static void stopAll();
	bool isRunning();

private:
	static void* workerThread(void *);

private:
	static BroadcastUtil* inst;
	static bool initialized;
	static pthread_mutex_t mtx;
	pthread_t* worker;
	RedisConnection* workerConn;
	std::string appId;
	std::string suffix;
	bool stop;
};


#endif /* INCLUDE_BROADCASTUTIL_HPP_ */
