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
#include <iostream>

BroadcastUtil* BroadcastUtil::inst = NULL;
bool BroadcastUtil::initialized = false;
pthread_mutex_t BroadcastUtil::mtx = pthread_mutex_t();

BroadcastUtil::~BroadcastUtil() {
	stop = true;
	pthread_join(*worker, NULL);
	delete workerConn;
	initialized = false;
}

bool BroadcastUtil::initializeId(const char * appId, int nodeId,
		RedisConnection * workerConn) {
#if __cplusplus >= 201103L
	return BroadcastUtil::initialize(appId, std::to_string(nodeId), workerConn);
#else
	return BroadcastUtil::initialize(appId,
			static_cast<std::ostringstream*>(&(std::ostringstream() << (nodeId)))->str().c_str(),
			workerConn);
#endif
}

bool BroadcastUtil::initialize(const char * appId, const char* salt,
		RedisConnection * workerConn) {
	if (!initialized) {
		if (pthread_mutex_init(&BroadcastUtil::mtx, NULL) != 0)
			return false;
		pthread_mutex_lock(&BroadcastUtil::mtx);
		try {
			// check once more, after we get lock, in case someone else had already initialized
			// while we were waiting
			if (!initialized && workerConn && workerConn->isConnected()) {
				inst = new BroadcastUtil();
				inst->stop = false;
				inst->appId = appId;
				if (salt == NULL) {
					// use current time as random salt for control channel suffix
#if __cplusplus >= 201103L
					inst->suffix = std::to_string(std::time(0));
#else
					inst->suffix =
							static_cast<std::ostringstream*>(&(std::ostringstream()
									<< (std::time(0))))->str();
#endif
				} else {
					inst->suffix = std::string(salt);
				}
				inst->controlChannel = std::string(NAMESPACE_PREFIX) + ":"
						+ inst->appId + CONTROL + inst->suffix;
				inst->workerConn = workerConn;
				inst->worker = new pthread_t();
				if (pthread_create(inst->worker, NULL, &workerThread, NULL)
						== 0) {
					initialized = true;
					// wait 1 second for worker thread to initialize
					sleep(1);
				} else {
					delete BroadcastUtil::inst;
				}
			} else if (workerConn) {
				delete workerConn;
			}
		} catch (...) {
			// failed to initialize
			initialized = false;
			if (inst)
				delete inst;
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

int BroadcastUtil::addSubscription(RedisConnection& conn,
		const char* channelName, callbackFunc func) {
	if (!BroadcastUtil::isRunning() || channelName == NULL
			|| std::strlen(channelName) == 0)
		return -1;

	// add callback to list of callback for this channel
	inst->myCallbacks[std::string(channelName)].insert(func);

	// send a control command to worker thread, to subscribe to this channel
	return BroadcastUtil::publish(conn, inst->controlChannel.c_str(),
			(ADD_COMMAND + channelName).c_str());
}

int BroadcastUtil::removeSubscription(RedisConnection& conn,
		const char* channelName) {
	if (!BroadcastUtil::isRunning() || channelName == NULL
			|| std::strlen(channelName) == 0)
		return -1;

	// remove all callbacks for this channel
	inst->myCallbacks[std::string(channelName)].clear();

	// send a control command to worker thread, to unsubscribe from this channel
	return BroadcastUtil::publish(conn, inst->controlChannel.c_str(),
			(REMOVE_COMMAND + channelName).c_str());
}

int BroadcastUtil::publish(RedisConnection& conn, const char* channelName,
		const char* message) {
	RedisResult res = conn.publish(channelName, message);
	if (res.resultType() == ERROR)
		std::cerr << "ERROR: " << res.errMsg() << std::endl;
	return res.intResult();
}
void BroadcastUtil::stopAll(RedisConnection& conn) {
	if (!BroadcastUtil::isRunning())
		return;
	BroadcastUtil::publish(conn, inst->controlChannel.c_str(),
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
		(*callback)(payload.c_str());
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
	while (inst->isRunning()) {
		// send nextCommand and block for message
		ChannelMessage message = ChannelMessage::from(
				BroadcastUtil::inst->workerConn->cmd(nextCommand.c_str()));
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
