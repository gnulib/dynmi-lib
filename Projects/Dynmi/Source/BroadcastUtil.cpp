/*
 * BroadcastUtil.cpp
 *
 *  Created on: Feb 24, 2017
 *      Author: bhadoria
 */

#include "Dynmi/BroadcastUtil.hpp"
#include "Dynmi/RedisConnection.hpp"
#include "Dynmi/RedisConnectionTL.hpp"
#include "Dynmi/RedisResult.hpp"
#include "Dynmi/ChannelMessage.hpp"
#include "Dynmi/DynmiGlobals.hpp"
#include <cstdlib>
#include <unistd.h>
#include <sstream>
#include <ctime>
#include <string>
#include <cstring>
#include <iostream>

BroadcastUtil* BroadcastUtil::inst = NULL;
bool BroadcastUtil::initialized = false;
bool BroadcastUtil::isTest = false;
pthread_mutex_t BroadcastUtil::mtx = PTHREAD_MUTEX_INITIALIZER;

BroadcastUtil::~BroadcastUtil() {
	if (isTest) return;
	// if it was a mock instance, return no-op
	if (inst != this) return;
	stop = true;
	pthread_join(*worker, NULL);
	initialized = false;
}

BroadcastUtil& BroadcastUtil::instance() {
	if (isTest) return *inst;
	static BroadcastUtil mock = BroadcastUtil();
	if (!initialized) {
		// return uninitialized instance and hope for best
		return mock;
	}
	return *inst;
}


bool BroadcastUtil::initializeWithId(const char * appId, int nodeId) {
#if __cplusplus >= 201103L
	return BroadcastUtil::initialize(appId, std::to_string(nodeId));
#else
	return BroadcastUtil::initialize(appId,
			static_cast<std::ostringstream*>(&(std::ostringstream() << (nodeId)))->str().c_str());
#endif
}

bool BroadcastUtil::initialize(BroadcastUtil* mock) {
	initialized = false;
	isTest = true;
	BroadcastUtil::inst = mock;
	return true;
}

bool BroadcastUtil::initialize(const char * appId, const char* salt) {
	if (!initialized) {
//		if (pthread_mutex_init(&BroadcastUtil::mtx, NULL) != 0) {
//			delete workerConn;
//			return false;
//		}
		pthread_mutex_lock(&BroadcastUtil::mtx);
		try {
			// check once more, after we get lock, in case someone else had already initialized
			// while we were waiting
			if (!initialized) {
				inst = new BroadcastUtil();
				inst->stop = false;
				inst->appId = appId;
				if (salt == NULL) {
					// use current time as random salt for control channel suffix
#if __cplusplus >= 201103L
					inst->nodeId = std::to_string(std::time(0));
#else
					inst->nodeId =
							static_cast<std::ostringstream*>(&(std::ostringstream()
									<< (std::time(0))))->str();
#endif
				} else {
					inst->nodeId = std::string(salt);
				}
				inst->controlChannel = std::string(NAMESPACE_PREFIX) + ":"
						+ inst->appId + CONTROL + inst->nodeId;
				inst->worker = new pthread_t();
				if (pthread_create(inst->worker, NULL, &workerThread, NULL)
						== 0) {
					initialized = true;
					// wait 1 second for worker thread to initialize
					sleep(1);
				} else {
					delete BroadcastUtil::inst;
				}
			}
		} catch (...) {
			// failed to initialize
			initialized = false;
			if (inst)
				delete inst;
			inst = NULL;
		}
		pthread_mutex_unlock(&BroadcastUtil::mtx);
	}
	return initialized;
}

std::string BroadcastUtil::getControlChannel() {
	return controlChannel;
}

int BroadcastUtil::addSubscription(const char* channelName, BroadcastUtil::callbackFunc func) {
	if (!BroadcastUtil::isRunning() || channelName == NULL
			|| std::strlen(channelName) == 0)
		return -1;

	// add callback to list of callback for this channel
	myCallbacks[std::string(channelName)].insert(func);

	// send a control command to worker thread, to subscribe to this channel
	return publish(inst->controlChannel.c_str(),
			(ADD_COMMAND + channelName).c_str());
}

int BroadcastUtil::removeSubscription(const char* channelName) {
	if (!BroadcastUtil::isRunning() || channelName == NULL
			|| std::strlen(channelName) == 0)
		return -1;

	// remove all callbacks for this channel
	myCallbacks[std::string(channelName)].clear();

	// send a control command to worker thread, to unsubscribe from this channel
	return publish(controlChannel.c_str(),
			(REMOVE_COMMAND + channelName).c_str());
}

int BroadcastUtil::publish(const char* channelName, const char* message) {
	RedisResult res = RedisConnectionTL::instance().publish(channelName, message);
	if (res.resultType() == ERROR || res.resultType() == FAILED)
		std::cerr << "ERROR: " << res.errMsg() << std::endl;
	return res.intResult();
}
void BroadcastUtil::stopAll() {
	if (!BroadcastUtil::isRunning())
		return;
	BroadcastUtil::inst->publish(inst->controlChannel.c_str(),
			STOP_COMMAND.c_str());
	delete BroadcastUtil::inst;
	BroadcastUtil::inst = NULL;
}

bool BroadcastUtil::isRunning() {
	return inst && !inst->stop;
}

void BroadcastUtil::notifyCallbacks(std::string channel, std::string payload) {
	// should this be a separate thread???
	std::set<callbackFunc> callbacks = inst->myCallbacks[channel];
	for (std::set<callbackFunc>::iterator callback = callbacks.begin();
			callback != callbacks.end(); ++callback) {
		(*callback)(channel.c_str(), payload.c_str());
	}
}

class ControlCommand {
public:
	std::string command;
	std::string arg;
	ControlCommand(std::string payload) {
		size_t spacePos = payload.find(" ");
		if (spacePos == std::string::npos) {
			command = payload.substr(0, payload.length());
		} else {
			command = payload.substr(0, spacePos + 1);
			arg = payload.substr(spacePos + 1, payload.length() - spacePos - 1);
		}
	}
};

void* BroadcastUtil::workerThread(void * arg) {
	static const std::string BLOCK = "";
	if (BroadcastUtil::inst == NULL || !BroadcastUtil::inst->isInitialized()) {
		return NULL;
	}
	// start by subscribing the control channel
	std::string nextCommand = SUBSCRIBE + inst->controlChannel;
	// create a clone of the RedisConnection, since same thread local instance will get used by registered callbacks
	RedisConnection workerConn = RedisConnectionTL::instance().clone();
	RedisConnection& conn = RedisConnectionTL::isInTest() ? RedisConnectionTL::instance() : workerConn;
	while (inst->isRunning()) {
		// send nextCommand and block for message
		ChannelMessage message = ChannelMessage::from(conn.cmd(nextCommand.c_str()));
		const std::string& channel = message.getChannelName();
		const std::string& payload = message.getMessage();
//		std::cerr << "CH[" << channel << "]:" << payload << ":" << std::endl;
		if (message.getType() == channelMessage::DATA) {
			// process any control messages...
			if (channel == inst->controlChannel) {
				// TODO do we really want to check each control
				// message below? or should we just simply pass through
				// "as is", so that its more flexible for future, only
				// the command sender method need to worry about sending
				// correct command
				ControlCommand control = ControlCommand(payload);
//				std::cerr << "CONTROL: |" << control.command << "|" << control.arg << "|" << std::endl;
				if (ADD_COMMAND == control.command) {
					nextCommand = SUBSCRIBE + control.arg;
//					std::cerr << "Adding subscription to channel: [" << control.arg << "]" << std::endl;
				} else if (REMOVE_COMMAND == control.command) {
					nextCommand = UNSUBSCRIBE + control.arg;
//					std::cerr << "Removing subscriptions from channel: [" << control.arg << "]" << std::endl;
				} else if (STOP_COMMAND == control.command) {
					// well, here is the reason why we need to check for control command
					inst->stop = true;
				} else {
					nextCommand = BLOCK;
				}
			} else {
				// notify all callbacks registered for this channel
				notifyCallbacks(channel, payload);
				nextCommand = BLOCK;
			}
		} else {
			// just wait for next data message
			nextCommand = BLOCK;
		}
	}
	return NULL;
}
