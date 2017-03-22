/*
 * MockRedisConnection.hpp
 *
 *  Created on: Mar 20, 2017
 *      Author: bhadoria
 */

#ifndef PROJECTS_DYNMI_TEST_MOCKREDISCONNECTION_HPP_
#define PROJECTS_DYNMI_TEST_MOCKREDISCONNECTION_HPP_

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "Dynmi/RedisConnection.hpp"

class MockRedisConnection : public RedisConnection {
public:
	MockRedisConnection(const char* host, int port) : RedisConnection(host, port){};
	MOCK_METHOD1(cmd, RedisResult(const char *));
	MOCK_METHOD2(publish, RedisResult(const char *, const char *));
	MOCK_METHOD2(cmdArgv, RedisResult(int, const std::string*));
	MOCK_CONST_METHOD0(isConnected, bool());
	MOCK_METHOD0(reconnect, bool());
	MOCK_CONST_METHOD0(clone, RedisConnection ());
};

#endif /* PROJECTS_DYNMI_TEST_MOCKREDISCONNECTION_HPP_ */
