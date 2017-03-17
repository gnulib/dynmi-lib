/*
 * CdMQMessage.cpp
 *
 *  Created on: Mar 16, 2017
 *      Author: bhadoria
 */

#include "CdMQMessage.hpp"
#include "CdMQUtil.hpp"

CdMQMessage::CdMQMessage(const std::string& data, const std::string& appId, const std::string& qName)
	: qName(qName), appId(appId), data(data), valid(true){
}

CdMQMessage::CdMQMessage()
	: valid(false){
}

CdMQMessage::~CdMQMessage() {
	if (valid) {
		CdMQUtil::unlock(*this);
	}
}

CdMQMessage::CdMQMessage(const CdMQMessage& other) {
	*this = other;
}

void CdMQMessage::operator=(const CdMQMessage& other) {
	this->data = other.data;
	this->qName = other.qName;
	this->appId = other.appId;
	this->valid = true;
	((CdMQMessage&)other).valid = false;
}
