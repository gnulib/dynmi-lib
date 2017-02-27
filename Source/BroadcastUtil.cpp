/*
 * BroadcastUtil.cpp
 *
 *  Created on: Feb 24, 2017
 *      Author: bhadoria
 */

#include "BroadcastUtil.hpp"
#include "RedisConnection.hpp"
#include "RedisResult.hpp"
#include "ChannelMessage.hpp"
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
	initialized = false;
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
				inst->controlChannel = std::string(NAMESPACE_PREFIX) + ":" + inst->appId + CONTROL + inst->suffix;
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

std::string BroadcastUtil::getControlChannel() {
	return controlChannel;
}

void BroadcastUtil::stopAll() {
	if (!BroadcastUtil::isInitialized()) return;
	// TODO: send control command to stop
	delete BroadcastUtil::inst;
	BroadcastUtil::inst = NULL;
}

bool BroadcastUtil::isRunning() {
	return inst && !inst->stop;
}

void* BroadcastUtil::workerThread(void * arg) {
	static const std::string BLOCK = "";
	if (BroadcastUtil::inst == NULL || !BroadcastUtil::inst->isInitialized()) {
		return NULL;
	}
	// start by subscribing the control channel
	std::string nextCommand = SUBSCRIBE + inst->controlChannel;
	while (inst->isRunning()) {
		// send nextCommand and block for message
		ChannelMessage message = ChannelMessage::from(BroadcastUtil::inst->workerConn->cmd(nextCommand.c_str()));
		const std::string& channel = message.getChannelName();
		const std::string& payload = message.getMessage();
		if (message.getType() == channelMessage::DATA) {
			// process any control messages...
			if (channel == inst->controlChannel) {
				// TODO do we really want to check each control
				// message below? or should we just simply pass through
				// "as is", so that its more flexible for future, only
				// the command sender method need to worry about sending
				// correct command
				if (payload.find(ADD_COMMAND) == 0) {
					nextCommand = SUBSCRIBE + payload.substr(ADD_COMMAND.length());
				} else if (payload.find(REMOVE_COMMAND) == 0) {
					nextCommand = UNSUBSCRIBE + payload.substr(REMOVE_COMMAND.length());
				} else if (payload.find(STOP_COMMAND) == 0) {
					nextCommand = UNSUBSCRIBE + payload.substr(STOP_COMMAND.length());
				} else {
					nextCommand = BLOCK;
				}
			} else {
				// notify all callbacks registered for this channel
				// TODO
				nextCommand = BLOCK;
			}
		} else {
			// just wait for next data message
			nextCommand = BLOCK;
		}
	}
	return NULL;
}
