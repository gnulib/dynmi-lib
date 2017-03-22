/*
 * CdMQUtil.cpp
 *
 *  Created on: Mar 16, 2017
 *      Author: bhadoria
 */



#include <string>
#include <sstream>
#include <iostream>
#include "Dynmi/CdMQUtil.hpp"
#include "Dynmi/CdMQMessage.hpp"
#include "Dynmi/InstancesUtil.hpp"
#include "Dynmi/BroadcastUtil.hpp"
#include "Dynmi/RedisConnectionTL.hpp"
#include "Dynmi/RedisConnection.hpp"
#include "Dynmi/RedisResult.hpp"
#include "Dynmi/DynmiGlobals.hpp"
#include "CdMQPayload.hpp"

const static std::string CDMQ = "CDMQ:APP:";
const static std::string QUEUE = ":QUEUE:";
const static std::string CHANNEL_QUEUE = QUEUE + "CHANNEL:";
const static std::string CHANNEL_LOCK = ":LOCK:CHANNEL:";
const static std::string SESSION_QUEUE = QUEUE + "SESSION:";
const static std::string SESSION_LOCK = ":LOCK:SESSION:";
const static std::string SESSION = ":SESSION:";
const static std::string ENQUEUE_CMD = "RPUSH";
const static std::string REM_CMD = "LREM";
const static std::string FETCH_CMD = "LINDEX ";
const static std::string GETALL_CMD = "LRANGE ";
const static std::string GETALL_INDX = " 0 -1";
const static std::string MARK_CMD = "LSET ";
const static std::string TOMBSTONE = "(:=DELETE-ME=:)";
const static int OP_TTL = 10;
static const std::string CHANNEL_ACTIVE = ":CHANNEL_ACTIVE";

CdMQUtil* CdMQUtil::inst = NULL;
bool CdMQUtil::initialized = false;
bool CdMQUtil::isTest = false;
std::string CdMQUtil::appId = "";
pthread_mutex_t CdMQUtil::mtx = PTHREAD_MUTEX_INITIALIZER;

CdMQUtil::~CdMQUtil() {
	if (inst == this) initialized = false;
}

bool CdMQUtil::initialize(const std::string& appId, const std::string& redisHost, const int redisPort) {
	if (!initialized) {
		pthread_mutex_lock(&CdMQUtil::mtx);
		try {
			// check once more, after we get lock, in case someone else had already initialized
			// while we were waiting
			if (!initialized) {
				CdMQUtil::inst = new CdMQUtil();
				CdMQUtil::inst->appId = appId;
				RedisConnectionTL::initialize(redisHost, redisPort);
				initialized = true;
			}
		} catch (...) {
			// failed to initialize
			initialized = false;
			if (CdMQUtil::inst)
				delete CdMQUtil::inst;
			CdMQUtil::inst = NULL;
		}
		pthread_mutex_unlock(&CdMQUtil::mtx);
	}
	return initialized;
}

bool CdMQUtil::initialize(CdMQUtil* mock) {
	isTest = true;
	inst = mock;
	initialized = false;
	return true;
}

CdMQUtil& CdMQUtil::instance() {
	if (isTest) return *inst;
	static CdMQUtil mock = CdMQUtil();
	if (!initialized) {
		// return uninitialized instance and hope for best
		return mock;
	}
	return *inst;
}

std::string makeChannelLockName(const std::string& appId, const std::string& qName) {
	return CDMQ + appId + CHANNEL_LOCK + qName;
}

std::string makeSessionLockName(const std::string& appId, const std::string& tag) {
	return CDMQ + appId + SESSION_LOCK + tag;
}

std::string makeChannelName(const std::string& appId, const std::string& qName) {
	return NAMESPACE_PREFIX + ":" + CDMQ + appId + CHANNEL_QUEUE + qName + CHANNEL_ACTIVE;
}

std::string makeSessionName(const std::string& appId, const std::string& tag) {
	return CDMQ + appId + SESSION + tag;
}

const std::string& makeChannelQueueName(const std::string& appId, const std::string& qName) {
	const static std::string name = NAMESPACE_PREFIX + ":" + CDMQ + appId + CHANNEL_QUEUE;
	return name;
}

std::string makeSessionQueueName(const std::string& appId, const std::string& qName) {
	return NAMESPACE_PREFIX + ":" + CDMQ + appId + SESSION_QUEUE + qName;
}

std::string makeFetchCommand(const std::string& appId, const std::string& qName, const int indx) {
#if __cplusplus >= 201103L
					return FETCH_CMD + makeChannelQueueName(appId, qName) + " " + std::to_string(indx);
#else
					return FETCH_CMD + makeChannelQueueName(appId, qName) + " " +
							static_cast<std::ostringstream*>(&(std::ostringstream()
									<< (indx)))->str();
#endif
}

std::string makeMarkerCommand(const std::string& qName, const int indx) {
#if __cplusplus >= 201103L
					return MARK_CMD + qName + " " + std::to_string(indx) + " " + TOMBSTONE;
#else
					return MARK_CMD + qName + " " +
							static_cast<std::ostringstream*>(&(std::ostringstream()
									<< (indx)))->str() + " " + TOMBSTONE;
#endif
}

bool CdMQUtil::unlock(CdMQMessage& message) {
	// release the lock on the session tag held by the message
//	if (message.valid && !message.tag.empty() && InstancesUtil::instance().releaseFastLock(RedisConnectionTL::instance(), appId.c_str(), makeSessionLockName(appId, message.tag).c_str()) != -1) {
	if (message.valid) {
		if (!message.tag.empty()) {
			InstancesUtil::instance().releaseFastLock(appId.c_str(), makeSessionLockName(appId, message.tag).c_str());
		}
		message.valid = false;
		// we finished processing a message for a channel, so publish a notification about channel being active
		BroadcastUtil::instance().publish(makeChannelName(appId, message.channelName).c_str(), message.channelName.c_str());
		return true;
	}
	return false;
}

bool CdMQUtil::enQueue(const std::string& appId, const std::string qName, const std::string& message, const std::string& tag) {
	bool result = false;
	// get a lock on the queue
	if (InstancesUtil::instance().getFastLock(appId.c_str(), makeChannelLockName(appId, qName).c_str(), OP_TTL) != 0) {
		////std::cerr << "did not get channel lock, skipping" << std::endl;
		return result;
	}
	//std::cerr << "got channel lock, proceeding" << std::endl;
	// add message to the back of the channel queue
	std::string args[3] = {ENQUEUE_CMD, makeChannelQueueName(appId, qName), CdMQPayload(qName, tag, message).toJson()};
	//std::cerr << "adding message to channel queue" << std::endl;
	RedisResult res = RedisConnectionTL::instance().cmdArgv(3, args);
	if (res.resultType() == INTEGER) {
		// send a notification about queue getting a message
		//std::cerr << "broadcasting event channel active after successful enqueue" << std::endl;
		BroadcastUtil::instance().publish(makeChannelName(appId, qName).c_str(), qName.c_str());
		result=true;
	} else {
		//std::cerr << "Failed to enQueue: " << res.errMsg() << std::endl;
	}
	// release lock on the queue
	InstancesUtil::instance().releaseFastLock(appId.c_str(), makeChannelLockName(appId, qName).c_str());
	return result;
}

CdMQMessage CdMQUtil::deQueue(const std::string qName, int ttl) {
	CdMQMessage message = CdMQMessage();
	// get a lock on the queue
	if (InstancesUtil::instance().getFastLock(appId.c_str(), makeChannelLockName(appId, qName).c_str(), OP_TTL) == 0) {
		//std::cerr << "got channel lock to deQueue, proceeding" << std::endl;
		// walk through all sessions in the session queue
		RedisResult res = RedisConnectionTL::instance().cmd((GETALL_CMD + makeChannelQueueName(appId, qName) + GETALL_INDX).c_str());
		if (res.resultType() == ARRAY) {
			//std::cerr << "got total " << res.arraySize() << " entries in queue" << std::endl;
			for (int i = 0; i < res.arraySize(); i++) {
				//std::cerr << "processing: " << res.arrayResult(i).strResult() << std::endl;
				CdMQPayload payload = CdMQPayload::fromJson(res.arrayResult(i).strResult());
				// attempt a lock on current message's session
				if (payload.isValid() && (payload.getTag().empty() || InstancesUtil::instance().getFastLock(appId.c_str(), makeSessionLockName(appId, payload.getTag()).c_str(), ttl) == 0)) {
					//std::cerr << "got session lock for " << payload.getTag() << ", proceeding" << std::endl;
					// construct CDMQ message object with valid indication
					message = CdMQMessage(payload.getMessage(), qName, payload.getTag());

					// RELIABLY remove the tag from session queue and message from channel queue
					// at the current i'th location as following:
					// first, set the item on i'th position as MARKER
					// then remove the occurrence of the item MARKER
					//
					// we use this approach instead of removing the item directly because
					// there is no guarantee that item (specially message) being removed is not
					// already in queue for some other locked tag that we skipped earlier
					// and if that was the case, then we'll end up deleting that wrong locked item
					{
						std::string name = makeChannelQueueName(appId, qName);
						//std::cerr << "marking tombstone: " << makeMarkerCommand(name, i) << std::endl;
						res = RedisConnectionTL::instance().cmd(makeMarkerCommand(name, i).c_str());
						if (res.resultType() == ERROR) {
							//std::cerr << "Failed to mark tombstones: " << res.errMsg() << std::endl;
						}
						std::string args[4] = {REM_CMD, name, "0", TOMBSTONE};
						res = RedisConnectionTL::instance().cmdArgv(4, args);
						if (res.resultType() == INTEGER) {
							//std::cerr << "removed total: " << res.intResult() << " tombstones" << std::endl;
						} else {
							//std::cerr << "Failed to remove tombstones: " << res.errMsg() << std::endl;
						}
					}
					// we found an unlocked message, break out of the loop
					break;
				}
			}
		}
		// release lock on the queue
		InstancesUtil::instance().releaseFastLock(appId.c_str(), makeChannelLockName(appId, qName).c_str());
		// lock on session will be removed when the message goes out of scope
	}
	return message;
}

void CdMQUtil::myCallbackFunc(const char* channel, const char* qName) {
	// check for notification type
	//std::cerr << "received channel active for " << qName << " on " << channel << std::endl;
	if (makeChannelName(appId, qName) == channel) {
		// fetch first unlocked message from the named channel
		CdMQMessage message = inst->deQueue(qName, OP_TTL);
		//std::cerr << "CdMQ: got message valid = " << message.isValid() << " payload \"" << message.getData() << "\"" << std::endl;

		// verify if this is a valid message
		if (!message.isValid()) return;

		// notify callback for the named channel with this message
		CdMQUtil::callbackFunc callback = inst->myCallbacks[qName];
		if (callback) {
			(*callback)(channel, message);
		}
	} else {
		// unknown notification
		//std::cerr << "unknown notification channel: " << channel << std::endl;
	}
}

bool CdMQUtil::registerReadyCallback(const std::string& appId, const std::string qName, CdMQUtil::callbackFunc callback) {
	// add consumer callback to map
	inst->myCallbacks[qName] = callback;
	//std::cerr << "added consumer call back to my map" << std::endl;

	// register my own callback to handle notifications for the named channel queue
	if (BroadcastUtil::instance().addSubscription(makeChannelName(appId, qName).c_str(), myCallbackFunc) != -1) {
		//std::cerr << "successfully registered callback with Broadcast util" << std::endl;
		return true;
	} else {
		//std::cerr << "failed to register callback with Broadcast util" << std::endl;
		return false;
	}
}
