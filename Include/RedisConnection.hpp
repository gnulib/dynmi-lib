/*
 * RedisConnection.hpp
 *
 *  Created on: Feb 21, 2017
 *      Author: bhadoria
 */

#ifndef REDISCONNECTION_HPP_
#define REDISCONNECTION_HPP_

// a namespace prefix to use with all our keys with Redis
// change this value to use different/custom app namespace
static const char * NAMESPACE_PREFIX = "SCALABLE_APP";

class RedisResult;
struct redisContext;
/**
 * The "RedisConnection" instance used to connect with redis is not thread safe.
 * Hence, we'll either have to use a connection pool per thread, or create a
 * new instance for each thread of execution where needed.
 */
class RedisConnection {
public:
	RedisConnection(const char *, int);
	virtual ~RedisConnection();

protected:
	RedisConnection();

public:
	// a generic method to execute redis commands
	virtual RedisResult cmd(const char *);

	// get connection status
	virtual bool isConnected() const;

protected:
	// my redis context pointer
	redisContext* myCtx;
};


#endif /* REDISCONNECTION_HPP_ */
