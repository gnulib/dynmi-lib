/*
 * CdMQUtil.cpp
 *
 *  Created on: Mar 16, 2017
 *      Author: bhadoria
 */



#include <string>
#include <sstream>
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
const static std::string CHANNEL_QUEUE = QUEUE + "CHANNEL:";
const static std::string CHANNEL_LOCK = ":LOCK:CHANNEL:";
const static std::string CHANNEL = ":CHANNEL:";
const static std::string SESSION_QUEUE = QUEUE + "SESSION:";
const static std::string SESSION_LOCK = ":LOCK:SESSION:";
const static std::string SESSION = ":SESSION:";
const static std::string ENQUEUE_CMD = "RPUSH ";
//const static std::string DEQUEUE_CMD = "LPOP ";
const static std::string UNQUEUE_CMD = "RPOP ";
const static std::string REM_CMD = "LREM ";
const static std::string FETCH_CMD = "LINDEX ";
const static std::string GETALL_CMD = "LRANGE ";
const static std::string GETALL_INDX = " 0 -1";
const static std::string MARK_CMD = "LSET ";
const static std::string MARKER = "-1!@#$$#@%^&&*";
const static int OP_TTL = 10;

CdMQUtil* CdMQUtil::inst = NULL;
bool CdMQUtil::initialized = false;
pthread_mutex_t CdMQUtil::mtx = PTHREAD_MUTEX_INITIALIZER;

CdMQUtil::~CdMQUtil() {
	if (inst == this) initialized = false;
}

bool CdMQUtil::initialize(const std::string& redisHost, const int redisPort) {
	if (!initialized) {
		CdMQUtil::initialize(new CdMQUtil());
		RedisConnectionTL::initialize(redisHost, redisPort);
	}
	return initialized;
}

bool CdMQUtil::initialize(CdMQUtil* inst) {
	if (!initialized) {
		pthread_mutex_lock(&CdMQUtil::mtx);
		try {
			// check once more, after we get lock, in case someone else had already initialized
			// while we were waiting
			if (!initialized) {
				CdMQUtil::inst = inst;
				initialized = true;
			}
		} catch (...) {
			// failed to initialize
			initialized = false;
			if (CdMQUtil::inst)
				delete CdMQUtil::inst;
			CdMQUtil::inst = NULL;
		}
		if (!initialized) {
			delete inst;
		}
		pthread_mutex_unlock(&CdMQUtil::mtx);
	}
	return initialized;
}

CdMQUtil& CdMQUtil::instance() {
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
	return CDMQ + appId + CHANNEL + qName;
}

std::string makeSessionName(const std::string& appId, const std::string& tag) {
	return CDMQ + appId + SESSION + tag;
}

std::string makeChannelQueueName(const std::string& appId, const std::string& qName) {
	return NAMESPACE_PREFIX + ":" + CDMQ + appId + CHANNEL_QUEUE + qName;
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
					return MARK_CMD + qName + " " + std::to_string(indx) + " " + MARKER;
#else
					return MARK_CMD + qName + " " +
							static_cast<std::ostringstream*>(&(std::ostringstream()
									<< (indx)))->str() + " " + MARKER;
#endif
}

bool CdMQUtil::unlock(CdMQMessage& message) {
	// release the lock on the session tag held by the message
	if (message.valid && !message.tag.empty() && InstancesUtil::instance().releaseFastLock(RedisConnectionTL::instance(), message.appId.c_str(), makeSessionLockName(message.appId, message.tag).c_str()) != -1) {
		message.valid = false;
		// we released the lock, so publish a notification about session being active
		// TODO

		return true;
	}
	return false;

}

bool CdMQUtil::enQueue(const std::string& appId, const std::string qName, const std::string& message, const std::string& tag) {
	bool result = false;
	// get a lock on the queue
	if (InstancesUtil::instance().getFastLock(RedisConnectionTL::instance(), appId.c_str(), makeChannelLockName(appId, qName).c_str(), OP_TTL) != 0) {
		return result;
	}
	// add message to the back of the channel queue
	std::string args[3] = {ENQUEUE_CMD, makeChannelQueueName(appId, qName), message};
	RedisResult res = RedisConnectionTL::instance().cmdArgv(3, args);
	if (res.resultType() != FAILED) {
		// add tag to the back of the session queue
		std::string args[3] = {ENQUEUE_CMD, makeSessionQueueName(appId, qName), tag};
		res = RedisConnectionTL::instance().cmdArgv(3, args);
		if (res.resultType() != FAILED) {
			// publish a message about queue getting a message
			// TODO

			result=true;
		} else {
			// remove the message from back of channel queue
			RedisConnectionTL::instance().cmd((UNQUEUE_CMD + makeChannelQueueName(appId, qName)).c_str());
		}
	}
	// release lock on the queue
	InstancesUtil::instance().releaseFastLock(RedisConnectionTL::instance(), appId.c_str(), makeChannelLockName(appId, qName).c_str());
	return result;
}

CdMQMessage CdMQUtil::deQueue(const std::string& appId, const std::string qName, int ttl) {
	CdMQMessage message = CdMQMessage();
	// get a lock on the queue
	if (InstancesUtil::instance().getFastLock(RedisConnectionTL::instance(), appId.c_str(), makeChannelLockName(appId, qName).c_str(), OP_TTL) == 0) {
		// walk through all sessions in the session queue
		RedisResult res = RedisConnectionTL::instance().cmd((GETALL_CMD + makeSessionQueueName(appId, qName) + GETALL_INDX).c_str());
		if (res.resultType() == ARRAY) {
			for (int i = 0; i < res.arraySize(); i++) {
				// attempt session lock for the session
				std::string tag = res.arrayResult(i).strResult();
				if (InstancesUtil::instance().getFastLock(RedisConnectionTL::instance(), appId.c_str(), makeSessionLockName(appId, tag).c_str(), ttl) == 0) {
					// fetch the message for this session
					res = RedisConnectionTL::instance().cmd(makeFetchCommand(appId, qName, i).c_str());
					if (res.resultType() == STRING) {
						// construct CDMQ message object with valid indication
						std::string payload = res.strResult();
						message = CdMQMessage(payload, appId, tag);

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
							std::string name = makeSessionQueueName(appId, qName);
							RedisConnectionTL::instance().cmd(makeMarkerCommand(name, i).c_str());
							std::string args[4] = {REM_CMD, name, "1", MARKER};
							RedisConnectionTL::instance().cmdArgv(4, args);
						}
						{
							std::string name = makeChannelQueueName(appId, qName);
							RedisConnectionTL::instance().cmd(makeMarkerCommand(name, i).c_str());
							std::string args[4] = {REM_CMD, name, "1", MARKER};
							RedisConnectionTL::instance().cmdArgv(4, args);
						}

						// register callback for any future session active events on this session
						// TODO, need to implement local call back management so can register dynamically for new session IDs
						break;
					} else {
						//
					}
				}
			}
		}
		// release lock on the queue
		InstancesUtil::instance().releaseFastLock(RedisConnectionTL::instance(), appId.c_str(), makeChannelLockName(appId, qName).c_str());
		// lock on session will be removed when the message goes out of scope
	}
	return message;
}

bool CdMQUtil::registerReadyCallback(const std::string& appId, const std::string qName, void (*callbackFunc)(const char*)) {
	return BroadcastUtil::instance().addSubscription(RedisConnectionTL::instance(), makeChannelName(appId, qName).c_str(), callbackFunc) != -1;
}
