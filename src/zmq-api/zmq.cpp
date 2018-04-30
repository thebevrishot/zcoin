#include "zmq-api/zmq.h"
#include "util.h"
#include <zmq.h>
#include <pthread.h>

void *zmqrcontext;
void *zmqrsocket;

void zmqError(const char *str)
{
    LogPrint("zmq", "zmq: Error: %s, errno=%s\n", str, zmq_strerror(errno));
}

// Internal function to send multipart message
static int zmq_send_multipart(void *sock, const void* data, size_t size, ...)
{
    va_list args;
    va_start(args, size);

    while (1)
    {
        zmq_msg_t msg;

        int rc = zmq_msg_init_size(&msg, size);
        if (rc != 0)
        {
            zmqError("Unable to initialize ZMQ msg");
            return -1;
        }

        void *buf = zmq_msg_data(&msg);
        memcpy(buf, data, size);

        data = va_arg(args, const void*);

        rc = zmq_msg_send(&msg, sock, data ? ZMQ_SNDMORE : 0);
        if (rc == -1)
        {
            zmqError("Unable to send ZMQ msg");
            zmq_msg_close(&msg);
            return -1;
        }

        zmq_msg_close(&msg);

        if (!data)
            break;

        size = va_arg(args, size_t);
    }
    return 0;
}

// TODO test this function
static int zmq_recv_multipart(void *sock,void* data,size_t* size)
{
	int64_t more;
	size_t more_size = sizeof(more);
	data = malloc(100*sizeof(*data));
	size_t idx;
	do {
		zmq_msg_t part;
		int rc = zmq_msg_init(&part);
		assert(rc == 0);
		rc = zmq_recvmsg(sock,&part,0);
		assert(rc != -1);
		rc = zmq_getsockopt(sock, ZMQ_RCVMORE, &more,&more_size);
		assert(rc == 0);
		size_t++;
	} while(more);

	*size = idx;

	return 0;
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
	while (!(needStopREQREPZMQ())) {
		// TODO
		// 1. get request message (call zmq_recv_multipart to read req)
		// 1.5 may need to check needStopREQREPZMQ() again for make sure
		// mutex wasn't call when waiting for input
		// 2. do something in tableZMQ (call tableZMQ.execute(method,args) in file
		// ./server.h u should get some value to reply)
		// 3. reply result that get from 2 (call zmq_send_multipart)
	}

	return (void*)true;
}

bool StartREQREPZMQ()
{
	LogPrint("zmq", "Starting REQ/REP ZMQ server\n");

	zmqrsocket = zmq_socket(zmqrcontext,ZMQ_ROUTER);
	if(!zmqrsocket){
		zmqError("Failed to create socket");
		return false;
	}

	// TODO read binding address from configuration
	int rc = zmq_bind(zmqrsocket, "tcp://*:5555");
	if (rc!=0)
	{
		zmqError("Unable to send ZMQ msg");
		zmq_close(zmqrsocket);
		return false;
	}

	// mutex mxq using for stop while loop
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
	zmq_close(zmqrsocket);
	pthread_mutex_unlock(&mxq);
}
