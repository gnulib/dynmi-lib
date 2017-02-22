/*
 * RedisResult.hpp
 *
 *  Created on: Feb 21, 2017
 *      Author: bhadoria
 */

#ifndef REDISRESULT_HPP_
#define REDISRESULT_HPP_

enum RedisResultType {
	INTEGER,
	STRING,
	ARRAY,
	NONE,
	ERROR
};
struct redisReply;

class RedisResult {
public:
	RedisResult();
	virtual ~RedisResult();
	RedisResult(const RedisResult&);
public:
	RedisResultType resultType() const {return type;}
	void reuse();
	const char* const strResult() const;
	int intResult() const;
	int arraySize() const;
	const RedisResult& arrayResult(int) const;
//	RedisResultType arrayResultType(int) const;
//	const char* const arrayStrResult(int) const;
//	int arrayIntResult(int) const;
	void setRedisReply(redisReply *);
	const char* errMsg() const;
	RedisResult& operator=(const RedisResult&);
private:
	void flush();
	static RedisResultType toMyType(int);
	void fromRedisReply(redisReply *);
	void setRedisReply(redisReply *, bool);
private:
	RedisResultType type;
//	redisReply *r;
	void* res;
	int size;
	static RedisResult noResult;
};



#endif /* REDISRESULT_HPP_ */
