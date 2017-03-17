/*
 * RedisConnectionTL.hpp
 *
 *  Created on: Mar 16, 2017
 *      Author: bhadoria
 */

#ifndef INCLUDE_REDISCONNECTIONTL_HPP_
#define INCLUDE_REDISCONNECTIONTL_HPP_

#include <pthread.h>
#include <string>
#include <map>

class RedisConnection;

class RedisConnectionTL {
private:
	RedisConnectionTL(){};
	virtual ~RedisConnectionTL() {}

public:
	static bool initialize(const std::string& redisHost, const int& redisPort);
	static RedisConnection& instance();

private:
	static bool initialized;
	static std::map<pthread_t, RedisConnection*> pool;
	static pthread_mutex_t mtx;
	static std::string redisHost;
	static int redisPort;
};


#endif /* INCLUDE_REDISCONNECTIONTL_HPP_ */
