/*
 * RedisConnection.cpp
 *
 *  Created on: Feb 21, 2017
 *      Author: bhadoria
 */

#include <cstddef>
#include <string>
#include "RedisConnection.hpp"
#include "RedisResult.hpp"
#include "hiredis/hiredis.h"
#include <iostream>

RedisConnection::RedisConnection(const char * host, int port) {
	myCtx = redisConnect(host, port);
	if (myCtx->err) {
		// handle error
		delete myCtx;
		myCtx = NULL;
	}
}

RedisConnection::RedisConnection() {
	myCtx = NULL;
}

RedisConnection::~RedisConnection() {
	if (myCtx)
		delete myCtx;
}

bool RedisConnection::isConnected() const {
	return myCtx != NULL;
}

RedisResult RedisConnection::cmd(const char * cmd) {
	RedisResult res = RedisResult();
	if (!myCtx) {
		// we don't have a connection, so nothing to do
		res.type = FAILED;
	} else {
		redisReply *r = (redisReply*) redisCommand(myCtx, cmd);
		if (r == NULL)
			res.type = FAILED;
		else
			res.setRedisReply(r);
	}
	return res;
}

static const char * PUBLISH = "PUBLISH";
RedisResult RedisConnection::publish(const char * channel, const char * message) {
	RedisResult res = RedisResult();
	if (!myCtx) {
		// we don't have a connection, so nothing to do
		res.type = FAILED;
	} else {
//		std::cerr << "##### sending command: " << PUBLISH << " |" << channel << "| |" << message << "|" << std::endl;
		const char * argv[3] = {PUBLISH, channel, message};
		redisReply *r = (redisReply*) redisCommandArgv(myCtx, 3, argv, NULL);
		if (r == NULL)
			res.type = FAILED;
		else
			res.setRedisReply(r);
	}
	return res;
}
