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
#include "InstancesUtil.hpp"

using namespace std;

void printResult(const RedisResult& res) {
	switch(res.resultType()) {
	case STRING:
		cout << "STRING: " << res.strResult() << endl;
		break;
	case INTEGER:
		cout << "INTEGER: " << res.intResult() << endl;
		break;
	case ARRAY:
		for (int i=0 ; i < res.arraySize() ; i++) {
			printResult((const RedisResult&) res.arrayResult(i));
		}
		break;
	case ERROR:
		cout << "ERROR: " << res.errMsg() << endl;
		break;
	default:
		cout << "<NULL>" << endl;
	}

}

int main(int argc, char **argv) {
	cout << "total args: " << argc << endl;
	for (int i =0; i <argc; i++) {
		cout << i << ": " << argv[i] << endl;
	}
	cout << "connecting to \"" << argv[1] << ":" << argv[2] << "\"" << endl;

	RedisConnection conn(argv[1], atoi(argv[2]));
	int id;
	if (!conn.isConnected()) {
		cout << "failed to connect." << endl;
		return -1;
	} else {
		id = InstancesUtil::getNewInstanceId(conn, "Test-App");
		cout << "my instance ID: " << id << endl;
		InstancesUtil::publishNodeDetails(conn, "Test-App",id, "localhost", 2039, 60);
	}
	bool done=false;
	bool isSubscribed = false;
	while (!done) {
		char message[1024];
		if (isSubscribed) {
			message[0] = 0;
		} else {
			cout << "[ID:" << id << "] CMD> ";
			std::cin.getline(message,1023);
			if (strstr(message, "subscribe") == message || strstr(message, "SUBSCRIBE") == message) {
				isSubscribed = true;
			} else if (strstr(message, "quit") == message || strstr(message, "QUIT") == message) {
				done = true;
				continue;
			}
		}
		RedisResult res = conn.cmd(message);
		if (res.resultType() == FAILED || !conn.isConnected()) {
			done = true;
		} else {
			printResult(res);
		}
	}
	if (InstancesUtil::removeNodeDetails(conn, "Test-App",id) == 0) {
		cout << "gracefully removed instance from system" << endl;
	} else {
		cout << "failed to remove instance from system" << endl;
	}
    return 0;
}
