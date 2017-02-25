/*
 * BroadcastUtil.cpp
 *
 *  Created on: Feb 24, 2017
 *      Author: bhadoria
 */

#include "BroadcastUtil.hpp"
#include "RedisConnection.hpp"
#include <cstdlib>
#include <unistd.h>
#include <sstream>
#include <string>
#include <iostream>

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
		int step = 0;
		std::cout << "##### Checkpoint: " << ++step << std::endl;
		pthread_mutex_lock(&BroadcastUtil::mtx);
		std::cout << "##### Checkpoint: " << ++step << std::endl;
		try {
			// check once more, after we get lock, in case someone else had already initialized
			// while we were waiting
			if (!initialized && workerConn && workerConn->isConnected()) {
				std::cout << "##### Checkpoint: " << ++step << std::endl;
				inst = new BroadcastUtil();
				std::cout << "##### Checkpoint: " << ++step << std::endl;
				inst->stop = false;
				std::cout << "##### Checkpoint: " << ++step << std::endl;
				inst->appId = appId;
#if __cplusplus >= 201103L
				inst->suffix = std::to_string(std::rand());
#else
				inst->suffix = static_cast<std::ostringstream*>( &(std::ostringstream() << (std::rand())) )->str();
#endif
				std::cout << "##### Checkpoint: " << ++step << std::endl;
				inst->workerConn = workerConn;
				std::cout << "##### Checkpoint: " << ++step << std::endl;
				inst->worker = new pthread_t();
				if(pthread_create(inst->worker, NULL, &workerThread, NULL) == 0) {
					std::cout << "##### SUCCESS Checkpoint: " << ++step << std::endl;
					initialized = true;
				} else {
					std::cout << "##### FAILED Checkpoint: " << ++step << std::endl;
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
		std::cout << "##### Checkpoint: " << ++step << std::endl;
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
	static const std::string CONTROL = "CHANNELS:CONTROL:";
	static const std::string MESSAGE = "CHANNELS:MESSAGE:";
	if (BroadcastUtil::inst == NULL || !BroadcastUtil::inst->isInitialized()) {
		return NULL;
	}
	while (inst->isRunning()) {
		sleep(1); // TODO: change this
	}
	return NULL;
}
