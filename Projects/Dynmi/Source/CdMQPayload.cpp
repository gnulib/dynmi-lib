/*
 * CdMQPayload.cpp
 *
 *  Created on: Mar 19, 2017
 *      Author: bhadoria
 */

#include "CdMQPayload.hpp"

static const std::string COLON = ":";
static const std::string QUOTE = "\"";
static const std::string COMMA = ",";
static const std::string START_OBJ = "{";
static const std::string END_OBJ = "}";
static const std::string TAG = QUOTE + "tag" + QUOTE;
static const std::string MESSAGE = QUOTE + "message" + QUOTE;

CdMQPayload CdMQPayload::fromJson(const std::string& json) {
	return CdMQPayload(json);
}

CdMQPayload::CdMQPayload(const std::string& tag, const std::string& message) {
	this->tag = tag;
	this->message = message;
	valid = true;
}

CdMQPayload::CdMQPayload(const CdMQPayload& other) {
	this->tag = other.tag;
	this->message = other.message;
	valid = other.valid;
}

CdMQPayload::CdMQPayload(const std::string& json) {
	valid = false;

	// find "{"
	size_t pos = json.find_first_of(START_OBJ);
	if (pos == std::string::npos) return;
	pos++;

	// find "tag"
	pos = json.find(TAG, pos);
	if (pos == std::string::npos) return;
	pos++;

	// find ":"
	pos = json.find_first_of(COLON, pos);
	if (pos == std::string::npos) return;
	pos++;

	// find starting quote for tag value
	pos = json.find_first_of(QUOTE, pos);
	if (pos == std::string::npos) return;
	pos++;

	// find ending quote for tag value
	size_t end = json.find_first_of(QUOTE, pos);
	if (end == std::string::npos) return;

	// extract tag from pos:end
	this->tag = json.substr(pos, end - pos);
	pos = end + 1;

	// find "message"
	pos = json.find(MESSAGE, pos);
	if (pos == std::string::npos) return;
	pos++;

	// find ":"
	pos = json.find_first_of(COLON, pos);
	if (pos == std::string::npos) return;
	pos++;

	// find starting quote for message value
	pos = json.find_first_of(QUOTE, pos);
	if (pos == std::string::npos) return;
	pos++;

	// find ending quote for message value
	end = json.find_last_of(END_OBJ);
	if (end == std::string::npos) return;
	end--;
	end = json.find_last_of(QUOTE, end);
	if (end == std::string::npos) return;

	// extract message from pos:end
	this->message = json.substr(pos, end - pos);

	valid = true;
}

std::string CdMQPayload::toJson() const {
	return START_OBJ + TAG + COLON + QUOTE + tag + QUOTE + COMMA + MESSAGE + COLON + QUOTE + message + QUOTE + END_OBJ;
}
