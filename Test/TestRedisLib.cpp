/*
 * TestRedisLib.cpp
 *
 *  Created on: Feb 21, 2017
 *      Author: bhadoria
 */



#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include "RedisConnection.hpp"
#include "RedisResult.hpp"
#include "InstancesUtil.hpp"
#include "BroadcastUtil.hpp"

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

string payload;
bool hasPayload = false;

void myNewInstanceCallback(const char* nodeId) {
	cerr << "NOTIFICATION: new instance [" << nodeId << "] for application started" << endl;
}

void myInstanceDownCallback(const char* nodeId) {
	cerr << "NOTIFICATION: instance [" << nodeId << "] for application stopped" << endl;
}

void myCallback(const char* msg) {
	cerr << "CALL BACK: " << msg << endl;
	payload = string(msg);
//	hasPayload = true;
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
		if (!BroadcastUtil::initialize("Test-App", id, new RedisConnection(argv[1], atoi(argv[2])))) {
			cout << "failed to initialize broadcast framework" << endl;
			return -1;
//		} else {
//			cout << "waiting 1 second for subscriber's worker thread to initialize" << endl;
//			sleep(1);
		}
		if (InstancesUtil::registerInstanceUpCallback(conn, "Test-App", myNewInstanceCallback) == -1) {
			cout << "failed to register callback for new instance notifications" << endl;
			return -1;
		}
		if (InstancesUtil::registerInstanceDownCallback(conn, "Test-App", myInstanceDownCallback) == -1) {
			cout << "failed to register callback for instance down notifications" << endl;
			return -1;
		}
	}
	bool done=false;
	bool isSubscribed = false;
	while (!done) {
		char message[1024];
		if (isSubscribed) {
			message[0] = 0;
		} else {
			if (hasPayload) {
				cerr << "CALL BACK: " << payload << endl;
				hasPayload = false;
			}
			cout << "[ID:" << id << "] CMD> ";
			std::cin.getline(message,1023);
			if (strstr(message, "subscribe") == message || strstr(message, "SUBSCRIBE") == message) {
				if (BroadcastUtil::addSubscription(conn, message+strlen("subscribe "), myCallback) != 1) {
					cout << "Failed to add subscription to channel [" << message+strlen("subscribe ") << "]" << endl;
				} else {
					cout << "Subscribed to channel [" << message+strlen("subscribe ") << "]" << endl;
				}
				continue;
			} else if (strstr(message, "unsubscribe") == message || strstr(message, "UNSUBSCRIBE") == message) {
				if (BroadcastUtil::removeSubscription(conn, message+strlen("unsubscribe ")) != 1) {
					cout << "Failed to remove subscription from channel [" << message+strlen("unsubscribe ") << "]" << endl;
				} else {
					cout << "Unsubscribed from channel [" << message+strlen("unsubscribe ") << "]" << endl;
				}
				continue;
			} else if (strstr(message, "publish") == message || strstr(message, "PUBLISH") == message) {
				string command = string(message);
				size_t channel = command.find(" ");
				size_t payload = command.find(" ", channel+1);
				cout << "publishing on channel [" << command.substr(channel+1, payload-channel-1) << "] : " << command.substr(payload+1) << endl;
				BroadcastUtil::publish(conn, command.substr(channel+1, payload-channel-1).c_str(), command.substr(payload+1).c_str());
				continue;
			} else if (strstr(message, "quit") == message || strstr(message, "QUIT") == message) {
				done = true;
				continue;
			} else if (strstr(message, "lock") == message || strstr(message, "LOCK") == message) {
				int res = InstancesUtil::getFastLock(conn, "Test-App", "test-lock", 60);
				if (res == 0) {
					cout << "Successful in getting lock!" << endl;
				} else if (res > 0) {
					cout << "Lock busy for next " << res << " seconds." << endl;
				} else {
					cout << "Failure in lock operation!!!" << endl;
				}
				continue;
			} else if (strstr(message, "unlock") == message || strstr(message, "UNLOCK") == message) {
				int res = InstancesUtil::releaseFastLock(conn, "Test-App", "test-lock");
				if (res == 0) {
					cout << "Successful in releasing the lock!" << endl;
				} else {
					cout << "Failure in lock operation!!!" << endl;
				}
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
	BroadcastUtil::stopAll(conn);
	if (InstancesUtil::removeNodeDetails(conn, "Test-App",id) == 0) {
		cout << "gracefully removed instance from system" << endl;
	} else {
		cout << "failed to remove instance from system" << endl;
	}
    return 0;
}
