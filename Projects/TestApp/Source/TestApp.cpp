/*
 * TestApp.cpp
 *
 *  Created on: Feb 21, 2017
 *      Author: bhadoria
 */



#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <sstream>
#include "Dynmi/RedisConnection.hpp"
#include "Dynmi/RedisResult.hpp"
#include "Dynmi/InstancesUtil.hpp"
#include "Dynmi/BroadcastUtil.hpp"
#include "Dynmi/CdMQUtil.hpp"
#include "Dynmi/CdMQMessage.hpp"
#include "Dynmi/RedisConnectionTL.hpp"

using namespace std;

int myNodeId;

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

void myNewInstanceCallback(const char* appId, const char* nodeId) {
	// skip if notification is about this instance
#if __cplusplus >= 201103L
	int id = stol(nodeId);
#else
	int id;
	istringstream(nodeId) >> id;
#endif
	if (id == myNodeId) return ;
	cerr << "NOTIFICATION: new instance [" << nodeId << "] for application [" << appId << "] started" << endl;
}

void myInstanceDownCallback(const char* appId, const char* nodeId) {
	cerr << "NOTIFICATION: instance [" << nodeId << "] for application [" << appId << "] stopped" << endl;
}

void myCallback(const char* channel, const char* msg) {
	cerr << "CALL BACK [" << channel << "]: " << msg << endl;
	payload = string(msg);
//	hasPayload = true;
}

void myCdMQCallback(const std::string& qName, const CdMQMessage& message) {
	cerr << "CdMQ: Channel [" << qName << "], is valid = " << message.isValid() << " payload \"" << message.getData() << "\"" << endl;
	sleep(2);
}

int main(int argc, char **argv) {
	cout << "total args: " << argc << endl;
	for (int i =0; i <argc; i++) {
		cout << i << ": " << argv[i] << endl;
	}
	if (argc != 3) {
		cout << "Usage: " << argv[0] << " <redis host> <redis port>" << endl;
		return -1;
	}
	cout << "connecting to \"" << argv[1] << ":" << argv[2] << "\"" << endl;


	RedisConnection conn(argv[1], atoi(argv[2]));
	if (!conn.isConnected()) {
		cout << "failed to connect." << endl;
		return -1;
	} else {
		InstancesUtil::initialize(argv[1], atoi(argv[2]));
		myNodeId = InstancesUtil::instance().getNewInstanceId(conn, "Test-App");
		cout << "my instance ID: " << myNodeId << endl;
		if (!BroadcastUtil::initializeWithId("Test-App", myNodeId, new RedisConnection(argv[1], atoi(argv[2])))) {
			cout << "failed to initialize broadcast framework" << endl;
			return -1;
		}
		if (InstancesUtil::instance().registerInstanceUpCallback(conn, "Test-App", myNewInstanceCallback) == -1) {
			BroadcastUtil::stopAll(conn);
			cout << "failed to register callback for new instance notifications" << endl;
			return -1;
		}
		if (InstancesUtil::instance().registerInstanceDownCallback(conn, "Test-App", myInstanceDownCallback) == -1) {
			BroadcastUtil::stopAll(conn);
			cout << "failed to register callback for instance down notifications" << endl;
			return -1;
		}
		if (InstancesUtil::instance().publishNodeDetails(conn, "Test-App",myNodeId, "localhost", 2039, 60) == -1) {
			BroadcastUtil::stopAll(conn);
			cout << "failed to publish instance details" << endl;
			return -1;
		}
		if (!CdMQUtil::initialize("Test-App", argv[1], atoi(argv[2]))) {
			BroadcastUtil::stopAll(conn);
			cout << "failed to initialize CdMQUtil" << endl;
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
			cout << "[ID:" << myNodeId << "] CMD> ";
			std::cin.getline(message,1023);
			if (strstr(message, "subscribe") == message || strstr(message, "SUBSCRIBE") == message) {
				if (BroadcastUtil::instance().addSubscription(conn, message+strlen("subscribe "), myCallback) != 1) {
					cout << "Failed to add subscription to channel [" << message+strlen("subscribe ") << "]" << endl;
				} else {
					cout << "Subscribed to channel [" << message+strlen("subscribe ") << "]" << endl;
				}
				continue;
			} else if (strstr(message, "unsubscribe") == message || strstr(message, "UNSUBSCRIBE") == message) {
				if (BroadcastUtil::instance().removeSubscription(conn, message+strlen("unsubscribe ")) != 1) {
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
				BroadcastUtil::instance().publish(conn, command.substr(channel+1, payload-channel-1).c_str(), command.substr(payload+1).c_str());
				continue;
			} else if (strstr(message, "quit") == message || strstr(message, "QUIT") == message) {
				done = true;
				continue;
			} else if (strstr(message, "lock") == message || strstr(message, "LOCK") == message) {
				int res = InstancesUtil::instance().getFastLock(conn, "Test-App", "test-lock", 60);
				if (res == 0) {
					cout << "Successful in getting lock!" << endl;
				} else if (res > 0) {
					cout << "Lock busy for next " << res << " seconds." << endl;
				} else {
					cout << "Failure in lock operation!!!" << endl;
				}
				continue;
			} else if (strstr(message, "unlock") == message || strstr(message, "UNLOCK") == message) {
				int res = InstancesUtil::instance().releaseFastLock(conn, "Test-App", "test-lock");
				if (res == 0) {
					cout << "Successful in releasing the lock!" << endl;
				} else {
					cout << "Failure in lock operation!!!" << endl;
				}
				continue;
			} else if (strstr(message, "send") == message || strstr(message, "SEND") == message) {
				string command = string(message);
				size_t channel = command.find(" ");
				size_t payload = command.find(" ", channel+1);
				cout << "sending to CdMQ channel [" << command.substr(channel+1, payload-channel-1) << "] : " << command.substr(payload+1) << endl;
				CdMQUtil::instance().registerReadyCallback("Test-App", command.substr(channel+1, payload-channel-1), myCdMQCallback);
				CdMQUtil::instance().enQueue("Test-App", command.substr(channel+1, payload-channel-1), command.substr(payload+1), "TAG1");
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
	if (InstancesUtil::instance().removeNodeDetails(conn, "Test-App",myNodeId) == 0) {
		cout << "gracefully removed instance from system" << endl;
	} else {
		cout << "failed to remove instance from system" << endl;
	}
	BroadcastUtil::stopAll(conn);
    return 0;
}
