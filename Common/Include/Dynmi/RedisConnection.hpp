/*
 * RedisConnection.hpp
 *
 *  Created on: Feb 21, 2017
 *      Author: bhadoria
 */

#ifndef REDISCONNECTION_HPP_
#define REDISCONNECTION_HPP_

#include <string>

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

	// a generic method to execute multi arg redis commands
	virtual RedisResult cmdArgv(int argc, const std::string* argv);

	// a method to support variable length strings in argument
	virtual RedisResult publish(const char * channel, const char* message);

	// get connection status
	virtual bool isConnected() const;

	// reconnect connection
	virtual bool reconnect();

	// clone a connection (for thread safety)
	virtual RedisConnection clone() const {return RedisConnection(redisHost.c_str(), redisPort);}

protected:
	// my redis context pointer
	redisContext* myCtx;
	// save redis info for reconnecting and cloning
	std::string redisHost;
	int redisPort;
};


#endif /* REDISCONNECTION_HPP_ */
