/*
 * CdMQUtil.cpp
 *
 *  Created on: Mar 16, 2017
 *      Author: bhadoria
 */



#include <string>
#include "CdMQUtil.hpp"
#include "CdMQMessage.hpp"
#include "InstancesUtil.hpp"
#include "BroadcastUtil.hpp"
#include "RedisConnectionTL.hpp"
#include "RedisConnection.hpp"
#include "RedisResult.hpp"
#include "DynmiGlobals.hpp"

const static std::string CDMQ = "CDMQ:APP:";
const static std::string QUEUE = ":QUEUE:";
const static std::string LOCK = ":LOCK:";
const static std::string CHANNEL = ":CHANNEL:";
const static std::string ENQUEUE_CMD = "RPUSH ";
const static std::string DEQUEUE_CMD = "LPOP ";

bool CdMQUtil::initialize(const std::string& redisHost, const int redisPort) {
	return RedisConnectionTL::initialize(redisHost, redisPort);
}

std::string makeLockName(const std::string& appId, const std::string& qName) {
	return CDMQ + appId + LOCK + qName;
}

std::string makeChannelName(const std::string& appId, const std::string& qName) {
	return CDMQ + appId + CHANNEL + qName;
}

bool CdMQUtil::unlock(CdMQMessage& message) {
	// release the lock on the queue held by the message
	if (message.valid && InstancesUtil::releaseFastLock(RedisConnectionTL::instance(), message.appId.c_str(), makeLockName(message.appId, message.qName).c_str()) != -1) {
		message.valid = false;
		// we released the lock, so publish a notification about queue becoming available
		// TODO

		return true;
	}
	return false;

}

bool CdMQUtil::enQueue(const std::string& appId, const std::string qName, const std::string& message) {
	// add message to the back of the queue
	std::string args[3] = {ENQUEUE_CMD, (ENQUEUE_CMD + NAMESPACE_PREFIX + ":" + CDMQ + appId + QUEUE + qName), message};
	RedisResult res = RedisConnectionTL::instance().cmdArgv(3, args);
	if (res.resultType() != FAILED) {
		// publish a message about queue getting a message
		// TODO

		return true;
	}
	return false;
}

CdMQMessage CdMQUtil::deQueue(const std::string& appId, const std::string qName, int ttl) {
	// get a lock on the queue
	if (InstancesUtil::getFastLock(RedisConnectionTL::instance(), appId.c_str(), makeLockName(appId, qName).c_str(), ttl) == 0) {
		// remove the first item from queue
		RedisResult res = RedisConnectionTL::instance().cmd((DEQUEUE_CMD + NAMESPACE_PREFIX + ":" + CDMQ + appId + QUEUE + qName).c_str());
		if (res.resultType() == STRING) {
			return CdMQMessage(std::string(res.strResult()), appId, qName);
		}
	}
	// we did not get a lock, so nothing to return back
	return CdMQMessage();
}

bool CdMQUtil::registerReadyCallback(const std::string& appId, const std::string qName, void (*callbackFunc)(const char*)) {
	return BroadcastUtil::addSubscription(RedisConnectionTL::instance(), makeChannelName(appId, qName).c_str(), callbackFunc) != -1;
}
