/*
 * RedisConnection.cpp
 *
 *  Created on: Feb 21, 2017
 *      Author: bhadoria
 */

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

int RedisConnection::cmd(const char * cmd, RedisResult& res) {
	// flush out any previous result from place holder
	res.reuse();
	if (!myCtx) {
		// we don't have a connection, so nothing to do
		return -1;
	}
	redisReply *r = (redisReply*) redisCommand(myCtx, cmd);
	if (r == NULL)
		return -1;
	res.setRedisReply(r);
	return 0;
}
