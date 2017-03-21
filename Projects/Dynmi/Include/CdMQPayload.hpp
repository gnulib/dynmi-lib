/*
 * CdMQPayload.hpp
 *
 *  Created on: Mar 19, 2017
 *      Author: bhadoria
 */

#ifndef PROJECTS_DYNMI_INCLUDE_CDMQPAYLOAD_HPP_
#define PROJECTS_DYNMI_INCLUDE_CDMQPAYLOAD_HPP_

#include <string>

class CdMQPayload {
protected:
	CdMQPayload();
	CdMQPayload(const std::string& json);

public:
	CdMQPayload(const CdMQPayload& other);
	CdMQPayload(const std::string& channel, const std::string& tag, const std::string& message);
	std::string toJson() const;
	static CdMQPayload fromJson(const std::string& json);

	bool isValid() const {return valid;}
	const std::string& getTag() const {return tag;}
	const std::string& getChannel() const {return channel;}
	const std::string& getMessage() const {return message;}

protected:
	std::string tag;
	std::string channel;
	std::string message;
	bool valid;
};


#endif /* PROJECTS_DYNMI_INCLUDE_CDMQPAYLOAD_HPP_ */
