#include "zmq-api/zmq.h"
#include "util.h"
#include <zmq.h>
#include <pthread.h>

void *zmqpcontext;
void *zmqpsocket;

void zmqError(const char *str)
{
    LogPrint("zmq", "zmq: Error: %s, errno=%s\n", str, zmq_strerror(errno));
}

pthread_mutex_t mxq;
int needStopREQREPZMQ(){
	switch(pthread_mutex_trylock(&mxq)) {
	case 0: /* if we got the lock, unlock and return 1 (true) */
		pthread_mutex_unlock(&mxq);
		return 1;
	case EBUSY: /* return 0 (false) if the mutex was locked */
		return 0;
	}
	return 1;
}

// arg[0] is the broker
static void* REQREP_ZMQ(void *arg)
{
	while (!(needStopREQREPZMQ)) {
		// TODO
		// 1. get request message
		// 2. do something in tableZMQ
		// 3. reply result
	}

	return (void*)true;
}

bool StartREQREPZMQ()
{
	LogPrint("zmq", "Starting REQ/REP ZMQ server\n");
	// TODO authentication

	zmqpsocket = zmq_socket(zmqpcontext,ZMQ_ROUTER);
	if(!zmqpsocket){
		zmqError("Failed to create socket");
		return false;
	}

	int rc = zmq_bind(zmqpsocket, "tcp://*:5555");
	if (rc == -1)
	{
		zmqError("Unable to send ZMQ msg");
		return false;
	}

	pthread_mutex_init(&mxq,NULL);
	pthread_mutex_lock(&mxq);
	pthread_t worker;
	pthread_create(&worker,NULL, REQREP_ZMQ,NULL);
}

void InterruptREQREPZMQ()
{
	LogPrint("zmq", "Interrupt REQ/REP ZMQ server\n");
}

void StopREQREPZMQ()
{
	LogPrint("zmq", "Stopping REQ/REP ZMQ server\n");
	pthread_mutex_unlock(&mxq);
}
