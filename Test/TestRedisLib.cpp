/*
 * TestRedisLib.cpp
 *
 *  Created on: Feb 21, 2017
 *      Author: bhadoria
 */



#include <stdlib.h>
#include <iostream>

#include "RedisConnection.hpp"
#include "RedisResult.hpp"

using namespace std;

void printResult(const RedisResult& res) {
	switch(res.resultType()) {
	case STRING:
		cout << res.strResult() << endl;
		break;
	case INTEGER:
		cout << res.intResult() << endl;
		break;
	case ARRAY:
		for (int i=0 ; i < res.arraySize() ; i++) {
			printResult((const RedisResult&) res.arrayResult(i));
		}
		break;
	case ERROR:
		cout << res.errMsg() << endl;
		break;
	default:
		cout << NULL << endl;
	}

}

int main(int argc, char **argv) {
	cout << "total args: " << argc << endl;
	for (int i =0; i <argc; i++) {
		cout << i << ": " << argv[i] << endl;
	}
	cout << "connecting to \"" << argv[1] << ":" << argv[2] << "\"" << endl;

	RedisConnection conn(argv[1], atoi(argv[2]));
	if (!conn.isConnected()) {
		cout << "failed to connect." << endl;
		return -1;
	}
	RedisResult res = RedisResult();
	bool done=false;
	while (!done) {
		cout << "CMD> ";
		char message[1024];
		std::cin.getline(message,1023);
		if (conn.cmd(message, res) != 0) {
			done = true;
		} else {
			printResult(res);
		}
	}
    return 0;
}
