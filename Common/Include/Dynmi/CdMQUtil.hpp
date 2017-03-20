/*
 * CdMQUtil.hpp
 *
 *  Created on: Mar 16, 2017
 *      Author: bhadoria
 */

#ifndef INCLUDE_CDMQUTIL_HPP_
#define INCLUDE_CDMQUTIL_HPP_

#include <pthread.h>
#include <string>
#include <map>

class CdMQMessage;

class CdMQUtil {
	friend class CdMQMessage;
protected:
	CdMQUtil() {}
	virtual ~CdMQUtil();

	// message will unlock the queue when its discarded
	static bool unlock(CdMQMessage& message);

	// my callback method for listening to all session and queue notifications
	static void myCallbackFunc(const char*, const char*);

public:
	// get an instance of the utility
	static CdMQUtil& instance();

	// initialize the queue
	static bool initialize(const std::string& appId, const std::string& redisHost, const int redisPort);

	// UT initialization from mock instance
	static bool initialize(CdMQUtil* mock);

	// add a message at the back of the named queue
	virtual bool enQueue(const std::string& appId, const std::string qName, const std::string& message, const std::string& tag="");

	// get a message from front of the named queue
	virtual CdMQMessage deQueue(const std::string qName, int ttl);

	typedef void (*callbackFunc)(const std::string& qName, const CdMQMessage& message);

	// register callback method to process message when available on named queue
	virtual bool registerReadyCallback(const std::string& appId, const std::string qName, CdMQUtil::callbackFunc);

private:
	static CdMQUtil* inst;
	static bool initialized;
	static bool isTest;
	static pthread_mutex_t mtx;
	static std::string appId;
	std::map<std::string,CdMQUtil::callbackFunc> myCallbacks;
};



#endif /* INCLUDE_CDMQUTIL_HPP_ */
