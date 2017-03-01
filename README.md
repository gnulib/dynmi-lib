# redis-lib
A library to provide primitives for implementing multi instance cloud scale applications.

## Overview
This library uses redis as an off-the-box distributed mem cache to provide following functionality:
* a wrapper over low level redis CRUD operations
* discovering application instances
* distributed mutex and leader selection between instances
* publishing broadcast messages to channels
* subscribing for message notifications on channels

## Getting started
Setup the repo as following:

1. clone repo from github url
1. cd to repo directory
1. update submodules: `git submodule update --init --recursive`
1. build library and test applications from top level repo directory: `make`

Above will build `hiredis`, `googletest` and `googlemock` dependencies, and then will build the primitives library `libredis`. All these artifacts will be under `Build` subdirectory under top level repo directory.

## Redis CRUD Operations
Library provides following utilities that take care of boiler plate code when dealing with low level redis operations:
* `RedisConnection` : a C++ container class to maintain redis conext and connection
* `RedisResult` : a C++ container class for accessing results of a low level redis operations

Above two are C++ wrappers over the `hiredis` library, and they take care of proper cleanup and pointers handling as and when objects are created/destroyed during execution. Note that `RedisConnection` is not thread safe. Hence an instance of the class needs to be created within the thread execution scope. Also, a reference to thread local instance of this class will need to be provided in all other operations provided by the library. 

## Instance Management
Library implements a singleton utility class `InstancesUtil` that provides following primitives for instance management:
* dynamic instance deployment and discovery:  
```
	// get a new ID for a newly deploying instance of the application
	static int getNewInstanceId(RedisConnection& conn, const char* appId);

	// register a callback method for any new instance notification
	static int registerInstanceUpCallback(RedisConnection& conn, const char* appId, void (*func)(const char*));

	// register a callback method for any instance down notification
	static int registerInstanceDownCallback(RedisConnection& conn, const char* appId, void (*func)(const char*));

	// publish a node's address details
	static int publishNodeDetails(RedisConnection& conn, const char* appId,
					const int nodeId, const char* host, int port, int ttl);

	// remove a node's details from system
	static int removeNodeDetails(RedisConnection& conn, const char* appId,
					const int nodeId);
```

* distributed synchronization and locking  
```
	// increment and get a counter
	static int incrCounter(RedisConnection& conn, const char* appId, const char* counter);

	// decrement and get a counter
	static int decrCounter(RedisConnection& conn, const char* appId, const char* counter);

	// get a fast lock on system
	static int getFastLock(RedisConnection& conn, const char* appId, const char* lockName, int ttl);

	// release the fast lock
	static int releaseFastLock(RedisConnection& conn, const char* appId, const char* lockName);
```

## Pub/Sub Messaging
Library implements a singleton class `BroadcastUtil` that provides following primitives for management and usage of message channels among all instances of the application:
* initialization and launch of background worker thread  
```
	// initialize the library before it can be used. we inject the RedisConnection instance
	// to be used by background worker thread for testability purpose
	static bool initialize(const char * appId, const char* uniqueId, RedisConnection * workerConn);

	// another way to initialize the library where instance's node ID is used as uniqueId
	static bool initializeId(const char * appId, int nodeId, RedisConnection * workerConn);

	// get initialization status
	static bool isInitialized() { return initialized;}

	// stop all subscriptions and worker thread for this channel, used during shutdown
	static void stopAll(RedisConnection& conn);

	// check if worker thread is running
	static bool isRunning();
```

* manage subscriptions to message channels  
```
	// subscribe this instance to receive messages published on named channel
	static int addSubscription(RedisConnection& conn, const char* channelName, callbackFunc);

	// remove subscription of this instance from named channel
	static int removeSubscription(RedisConnection& conn, const char* channelName);
```

* broadcast/publish a message to a channel  
```
	// publish a message to broadcast on named channel
	static int publish(RedisConnection& conn, const char* channelName, const char* message);
```

## Example application
An very simple test application is provided that has some basic usage of the primitives described above. Its built along with rest of the library code and executable is under `Build/test-app` after building the repo. It can be used as following:
* launching application  
```
$ ./Build/test-app localhost 6379
total args: 3
0: ./Build/redis-cli
1: localhost
2: 6379
connecting to "localhost:6379"
my instance ID: 139
NOTIFICATION: new instance [139] for application started
[ID:139] CMD> 
```

* basic redis operartions  
```
[ID:139] CMD> set foo bar
<NULL>
[ID:139] CMD> get foo
STRING: bar
[ID:139] CMD> 
```

* adding subscriptions to a channel  
```
[ID:139] CMD> subscribe test_channel
Subscribed to channel [test_channel]
[ID:139] CMD> 
```

* publishing message to a channel  
```
[ID:139] CMD> publish test_channel hi how are you?
publishing on channel [test_channel] : hi how are you?
[ID:139] CMD> CALL BACK: hi how are you?
```

* lock/unlock a named mutex  
```
[ID:139] CMD> lock test_lock
Successful in getting lock!
[ID:139] CMD> lock test_lock
Lock busy for next 56 seconds.
[ID:139] CMD> unlock test_lock
Successful in releasing the lock!
[ID:139] CMD> lock test_lock
Successful in getting lock!
[ID:139] CMD> 
```

* quit application  
```
[ID:139] CMD> quit
NOTIFICATION: instance [139] for application stopped
gracefully removed instance from system
$
```

For testing purpose, multiple instances of this application can be executed in parallel, to understand the behavior of certain multi-instance primitives, e.g. lock management, or channel subscription etc.
