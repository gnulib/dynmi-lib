/*
 * BroadcastUtil.cpp
 *
 *  Created on: Feb 24, 2017
 *      Author: bhadoria
 */

#include "BroadcastUtil.hpp"
#include "RedisConnection.hpp"
#include "RedisResult.hpp"
#include <cstdlib>
#include <unistd.h>
#include <sstream>
#include <string>

BroadcastUtil* BroadcastUtil::inst = NULL;
bool BroadcastUtil::initialized = false;
pthread_mutex_t BroadcastUtil::mtx = pthread_mutex_t();

BroadcastUtil::~BroadcastUtil() {
	stop = true;
	pthread_join(*worker, NULL);
	delete workerConn;
}
bool BroadcastUtil::initialize(const char * appId, RedisConnection * workerConn) {
	if (!initialized) {
		if (pthread_mutex_init(&BroadcastUtil::mtx, NULL) != 0) return false;
		pthread_mutex_lock(&BroadcastUtil::mtx);
		try {
			// check once more, after we get lock, in case someone else had already initialized
			// while we were waiting
			if (!initialized && workerConn && workerConn->isConnected()) {
				inst = new BroadcastUtil();
				inst->stop = false;
				inst->appId = appId;
#if __cplusplus >= 201103L
				inst->suffix = std::to_string(std::rand());
#else
				inst->suffix = static_cast<std::ostringstream*>( &(std::ostringstream() << (std::rand())) )->str();
#endif
				inst->workerConn = workerConn;
				inst->worker = new pthread_t();
				if(pthread_create(inst->worker, NULL, &workerThread, NULL) == 0) {
					initialized = true;
				} else {
					delete BroadcastUtil::inst;
				}
			} else if (workerConn) {
				delete workerConn;
			}
		} catch (...) {
			// failed to initialize
			initialized = false;
			if (inst) delete inst;
			inst = NULL;
		}
		pthread_mutex_unlock(&BroadcastUtil::mtx);
	} else if (workerConn) {
		delete workerConn;
	}
	return initialized;
}

void BroadcastUtil::stopAll() {
	if (!BroadcastUtil::isInitialized()) return;
	// TODO: send control command to stop
	delete BroadcastUtil::inst;
	BroadcastUtil::inst = NULL;
}
bool BroadcastUtil::isRunning() {
	return !stop;
}

void* BroadcastUtil::workerThread(void * arg) {
	static const std::string CONTROL = ":CHANNELS:CONTROL:";
	static const std::string MESSAGE = ":CHANNELS:MESSAGE:";
	static const std::string SUBSCRIBE = "SUBSCRIBE ";
	static const std::string NAMESPACE = std::string(NAMESPACE_PREFIX) + ":";
	if (BroadcastUtil::inst == NULL || !BroadcastUtil::inst->isInitialized()) {
		return NULL;
	}
	// start by subscribing the control channel
	RedisResult res = BroadcastUtil::inst->workerConn->cmd((SUBSCRIBE + NAMESPACE + inst->appId + CONTROL).c_str());
	while (inst->isRunning()) {
		// process result

		sleep(1); // TODO: change this
	}
	return NULL;
}
