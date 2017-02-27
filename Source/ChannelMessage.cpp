/*
 * ChannelMessage.cpp
 *
 *  Created on: Feb 26, 2017
 *      Author: bhadoria
 */


#include "ChannelMessage.hpp"
#include "RedisResult.hpp"
#include <sstream>

using namespace std;

//ChannelMessage::ChannelMessage() {}

//ChannelMessage::~ChannelMessage() {}

static const string TYPE_DATA = "message";
static const string TYPE_SUBSCRIBE = "subscribe";
static const string TYPE_UNSUBSCRIBE = "unsubscribe";

ChannelMessage ChannelMessage::from(const RedisResult& response) {
	ChannelMessage message = ChannelMessage();
	if (response.resultType() == ARRAY && response.arraySize() == 3
			&& response.arrayResult(0).resultType() == STRING
			&& response.arrayResult(1).resultType() == STRING ) {
		if (TYPE_DATA == response.arrayResult(0).strResult()
				&& response.arrayResult(2).resultType() == STRING) {
			message.channelName = string(response.arrayResult(1).strResult());
			message.message = string(response.arrayResult(2).strResult());
			message.type = channelMessage::DATA;
		} else if ((TYPE_SUBSCRIBE == response.arrayResult(0).strResult()
				|| TYPE_UNSUBSCRIBE == response.arrayResult(0).strResult())
				&& response.arrayResult(2).resultType() == INTEGER) {
			message.channelName = string(response.arrayResult(1).strResult());
#if __cplusplus >= 201103L
			message.message = std::to_string(response.arrayResult(2).intResult());
#else
			message.message = static_cast<std::ostringstream*>( &(std::ostringstream() << (response.arrayResult(2).intResult())) )->str();
#endif
			message.type = (TYPE_SUBSCRIBE == response.arrayResult(0).strResult())
					? channelMessage::SUBSCRIBE : channelMessage::UNSUBSCRIBE;
		}
	}
	return message;
}

const string& ChannelMessage::getChannelName() const {
	return channelName;
}

const string& ChannelMessage::getMessage() const {
	return message;
}
