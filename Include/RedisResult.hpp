/*
 * RedisResult.hpp
 *
 *  Created on: Feb 21, 2017
 *      Author: bhadoria
 */

#ifndef REDISRESULT_HPP_
#define REDISRESULT_HPP_

struct redisReply;

enum RedisResultType {
	INTEGER,
	STRING,
	ARRAY,
	NONE,
	ERROR
};

class RedisResult {
public:
	RedisResult();
	virtual ~RedisResult();
	RedisResult(const RedisResult&);
public:
	enum RedisResultType resultType() const {return type;}
	void reuse();
	const char* const strResult() const;
	int intResult() const;
	int arraySize() const;
	const RedisResult& arrayResult(int) const;
	void setRedisReply(redisReply *);
	const char* errMsg() const;
	RedisResult& operator=(const RedisResult&);
private:
	void flush();
	static enum RedisResultType toMyType(int);
	void fromRedisReply(redisReply *);
	void setRedisReply(redisReply *, bool);
private:
	enum RedisResultType type;
	void* res;
	int size;
	static RedisResult noResult;
};



#endif /* REDISRESULT_HPP_ */
