//
// Created by AriannaK97 on 11/6/20.
//

#ifndef WHOSERVERPROGRAM_SERVERIO_H
#define WHOSERVERPROGRAM_SERVERIO_H

#include <stdbool.h>
#include <pthread.h>
#include <netinet/in.h>
#include "structs.h"

#define MESSAGE_BUFFER 120

typedef struct ServerInputArgs{
    uint16_t queryPortNum;
    uint16_t statisticsPortNum;
    int numThreads;
    int bufferSize;
}ServerInputArgs;

typedef struct CircularBuffer{
    int *buffer;
    size_t head;
    size_t tail;
    size_t max; //of the buffer
    bool full;
}CircularBuffer;

typedef struct ThreadPool{
    pthread_t* threads;
    int numberOfThreads;
    pthread_mutex_t mutexLock;
    pthread_cond_t mutexCond;
    pthread_cond_t mutexCond2;
    int end;
    CircularBuffer *circularBuffer;
    uint32_t localIP;
    uint16_t localPort;
}ThreadPool;

typedef struct Socket{
    int socket;
    struct sockaddr_in socketAddressServer;
    struct sockaddr_in socketAddressClient;
    struct sockaddr *serverptr;
    struct sockaddr *clientptr;
    int socketSize;
    int type;
}Socket;

typedef struct WorkerItem{
    int workerPort;
    struct List *list;
}WorkerItem;

typedef struct WhoServerManager{
    uint16_t queryPortNum;
    uint16_t statisticsPortNum;
    int numThreads;
    int bufferSize;
    Socket *sock;
    int numOfWorkers;
    WorkerItem *workerItemArray;
    int workerArrayIndex;
    pthread_mutex_t mtxW;
}WhoServerManager;

typedef struct CountryListItem{
    char *country;
}CountryListItem;

enum Type{
    WORKER_SOCKET,
    CLIENT_SOCKET
};

/*Global variables*/
//ThreadPool *threadPool;
WhoServerManager *whoServerManager;


ServerInputArgs* getWhoServerArguments(int argc, char** argv);

WhoServerManager* initializeWhoServerManager(ServerInputArgs* serverInputArgs);

void sigchld_handler (int sig);

void perror_exit(char *message);

bool receiveStats(int readBufferSize,int sock);

CircularBuffer* circularBufInit(size_t size);

void circularBufFree(CircularBuffer *cbuf);

void circularBufReset(CircularBuffer *cbuf);

void circularBufPut(CircularBuffer *cbuf, int data);

int circularBufPut2(CircularBuffer *cbuf, int data);

int circularBufGet(CircularBuffer *cbuf, int * data);

bool circularBufEmpty(CircularBuffer *cbuf);

bool circularBufFull(CircularBuffer *cbuf);

size_t circularBufCapacity(CircularBuffer *cbuf);

void advancePointer(CircularBuffer *cbuf);

void retreatPointer(CircularBuffer *cbuf);

size_t circularBufSize(CircularBuffer *cbuf);

ThreadPool* initializeThreadpool(int numberOfThreads, CircularBuffer* buffer, uint32_t ip, uint16_t port);

Socket* initializeSocket(uint16_t port, int type);

void * workerThread(void* arg);

#endif //WHOSERVERPROGRAM_SERVERIO_H
