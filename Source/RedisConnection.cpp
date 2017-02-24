/*
 * RedisConnection.cpp
 *
 *  Created on: Feb 21, 2017
 *      Author: bhadoria
 */

#include <cstddef>
#include "RedisConnection.hpp"
#include "RedisResult.hpp"
#include "hiredis/hiredis.h"

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
		res.type = ERROR;
	} else {
		redisReply *r = (redisReply*) redisCommand(myCtx, cmd);
		if (r == NULL)
			res.type = ERROR;
		else
			res.setRedisReply(r);
	}
	return res;
}
