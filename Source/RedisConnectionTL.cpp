/*
 * RedisConnectionTL.cpp
 *
 *  Created on: Mar 16, 2017
 *      Author: bhadoria
 */

#include "RedisConnectionTL.hpp"
#include "RedisConnection.hpp"

bool RedisConnectionTL::initialized = false;
bool RedisConnectionTL::inTest = false;
RedisConnection* RedisConnectionTL::mock = NULL;
pthread_mutex_t RedisConnectionTL::mtx = pthread_mutex_t();
std::map<pthread_t, RedisConnection*> RedisConnectionTL::pool = std::map<pthread_t, RedisConnection*>();
std::string RedisConnectionTL::redisHost = "";
int RedisConnectionTL::redisPort = 0;

bool RedisConnectionTL::initializeTest(RedisConnection* mock) {
	initialized = true;
	inTest = true;
	RedisConnectionTL::mock = mock;
	return true;
}

bool RedisConnectionTL::initialize(const std::string& redisHost, const int& redisPort) {
	if (!RedisConnectionTL::initialized) {
		if (pthread_mutex_init(&RedisConnectionTL::mtx, NULL) != 0) {
			return false;
		}
		pthread_mutex_lock(&RedisConnectionTL::mtx);
		// check once more after we get lock
		if (!RedisConnectionTL::initialized) {
			try {
				RedisConnectionTL::redisHost = redisHost;
				RedisConnectionTL::redisPort = redisPort;
				initialized = true;
			} catch (...) {
				// failed to initialize
				initialized = false;
			}
		}
		pthread_mutex_unlock(&RedisConnectionTL::mtx);
	}
	return initialized;
}

RedisConnection& RedisConnectionTL::instance() {
	if (inTest) return *RedisConnectionTL::mock;
	pthread_t tId = pthread_self();
	RedisConnection* conn = RedisConnectionTL::pool[tId];
	if (!conn) {
		pthread_mutex_lock(&RedisConnectionTL::mtx);
		// check once more after we get lock
		conn = RedisConnectionTL::pool[tId];
		if (!conn) {
			try {
				conn = new RedisConnection(RedisConnectionTL::redisHost.c_str(), RedisConnectionTL::redisPort);
				RedisConnectionTL::pool[tId] = conn;
			} catch (...) {
				// failed to initialize thread local instance
				static RedisConnection backup = RedisConnection(RedisConnectionTL::redisHost.c_str(), RedisConnectionTL::redisPort);
				conn = &backup;
			}
		}
		pthread_mutex_unlock(&RedisConnectionTL::mtx);
	}
	return *conn;
}
