/*
 * CdMQMessage.hpp
 *
 *  Created on: Mar 16, 2017
 *      Author: bhadoria
 */

#ifndef INCLUDE_CDMQMESSAGE_HPP_
#define INCLUDE_CDMQMESSAGE_HPP_

#include <string>

class CdMQUtil;

class CdMQMessage {
	friend class CdMQUtil;

protected:
	CdMQMessage();
	// only the CdMQUtil can create a new message instance
	// because it will also lock the queue with this message
	CdMQMessage(const std::string& data, const std::string& appId, const std::string& tag);

public:
	// when object goes out of scope, if it holds the lock on queue then
	// lock will be released
	virtual ~CdMQMessage();

	// copy constructor to pass message around
	// this will also transfer the queue lock ownership with the copy
	CdMQMessage(const CdMQMessage& other);
	void operator=(const CdMQMessage& other);

	// get data
	const std::string& getData() const {return data;};

	// get status, if DTO is valid
	// which also means this instance has the lock on the queue
	bool isValid() const {return valid;}

private:
	std::string tag;
	std::string appId;
	std::string data;
	bool valid;
};


#endif /* INCLUDE_CDMQMESSAGE_HPP_ */
