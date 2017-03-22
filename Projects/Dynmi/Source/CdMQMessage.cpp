/*
 * CdMQMessage.cpp
 *
 *  Created on: Mar 16, 2017
 *      Author: bhadoria
 */

#include "Dynmi/CdMQMessage.hpp"
#include "Dynmi/CdMQUtil.hpp"

CdMQMessage::CdMQMessage(const std::string& data, const std::string& channelName, const std::string& tag)
	: tag(tag), channelName(channelName), data(data), valid(true){
}

CdMQMessage::CdMQMessage()
	: valid(false){
}

CdMQMessage::~CdMQMessage() {
	if (valid) {
		CdMQUtil::instance().unlock(*this);
	}
}

CdMQMessage::CdMQMessage(const CdMQMessage& other) {
	*this = other;
}

void CdMQMessage::operator=(const CdMQMessage& other) {
	this->data = other.data;
	this->channelName = other.channelName;
	this->tag = other.tag;
	this->valid = true;
	((CdMQMessage&)other).valid = false;
}
