/*
 * CdMQUtil.hpp
 *
 *  Created on: Mar 16, 2017
 *      Author: bhadoria
 */

#ifndef INCLUDE_CDMQUTIL_HPP_
#define INCLUDE_CDMQUTIL_HPP_

class CdMQMessage;

class CdMQUtil {
	friend class CdMQMessage;
protected:
	CdMQUtil() {}
	virtual ~CdMQUtil() {}

	// message will unlock the queue when its discarded
	static bool unlock(CdMQMessage& message);

public:
	// initialize the queue
	static bool initialize(const std::string& redisHost, const int redisPort);

	// add a message at the back of the named queue
	static bool enQueue(const std::string& appId, const std::string qName, const std::string& message);

	// get a message from front of the named queue
	static CdMQMessage deQueue(const std::string& appId, const std::string qName, int ttl);

	// register queue ready notifier
	static bool registerReadyCallback(const std::string& appId, const std::string qName, void (*callbackFunc)(const char*));
};



#endif /* INCLUDE_CDMQUTIL_HPP_ */
