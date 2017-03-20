/*
 * ChannelMessage.hpp
 *
 *  Created on: Feb 26, 2017
 *      Author: bhadoria
 */

#ifndef INCLUDE_CHANNELMESSAGE_HPP_
#define INCLUDE_CHANNELMESSAGE_HPP_
#include <string>

class RedisResult;

namespace channelMessage {
enum ChannelMessageType {
	SUBSCRIBE,
	UNSUBSCRIBE,
	DATA,
	ERROR
};
}

// a container class to de-serialize messages received by subscriber
class ChannelMessage {
private:
	ChannelMessage() {type = channelMessage::ERROR;}

public:
	~ChannelMessage() {};
	static ChannelMessage from(const RedisResult& response);
	const std::string& getChannelName() const;
	const std::string& getMessage() const;
	const channelMessage::ChannelMessageType getType() const {return type;}

private:
	std::string channelName;
	std::string message;
	channelMessage::ChannelMessageType type;
};



#endif /* INCLUDE_CHANNELMESSAGE_HPP_ */
