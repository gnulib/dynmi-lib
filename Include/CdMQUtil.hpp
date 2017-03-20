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

class CdMQMessage;

class CdMQUtil {
	friend class CdMQMessage;
protected:
	CdMQUtil() {}
	virtual ~CdMQUtil() {initialized = false;}

	// message will unlock the queue when its discarded
	static bool unlock(CdMQMessage& message);

public:
	// get an instance of the utility
	CdMQUtil& instance();

	// initialize the queue
	static bool initialize(const std::string& redisHost, const int redisPort);

	// UT initialization from mock instance
	static bool initialize(CdMQUtil* uut);

	// add a message at the back of the named queue
	bool enQueue(const std::string& appId, const std::string qName, const std::string& message, const std::string& tag="");

	// get a message from front of the named queue
	CdMQMessage deQueue(const std::string& appId, const std::string qName, int ttl);

	// register callback method to process message when available on named queue
	bool registerReadyCallback(const std::string& appId, const std::string qName, void (*callbackFunc)(const char*));

private:
	static CdMQUtil* inst;
	static bool initialized;
	static pthread_mutex_t mtx;
};



#endif /* INCLUDE_CDMQUTIL_HPP_ */
