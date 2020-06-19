//
// Created by AriannaK97 on 18/6/20.
//

#ifndef WHOCLIENT_WHOCLIENTIO_H
#define WHOCLIENT_WHOCLIENTIO_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include "whoClientCircularBuffer.h"
#include <bits/types/FILE.h>
#include <netinet/in.h>

#define DATA_LENGTH 120

/**
 * Structures
 * */

typedef struct Socket{
    int socket;
    struct sockaddr_in socketAddressServer;
    struct sockaddr_in socketAddressClient;
    int socketSize;
}Socket;

typedef struct WhoClientInputArguments{
    char *queryFile;
    int numThreads;
    uint16_t servPort;
    char *servIP;
}WhoClientInputArguments;

typedef struct WhoClientManager{
    char *queryFile;
    int numThreads;
    uint16_t servPort;
    char *servIP;
    Socket *serverSocket;
    Socket *clientSocket;
}WhoClientManager;

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


/**
 * Global variables
 * */
WhoClientManager *whoClientManager;


/**
 * Function Declaration
 * */

WhoClientInputArguments* getWhoClientArguments(int argc, char** argv);

WhoClientManager* initializeWhoClientManager(WhoClientInputArguments* serverInputArgs);

void freeWhoClientInputArgs(WhoClientInputArguments *arguments);

int getNumOfLinesInFile(FILE* file);

void readQueryFile(ThreadPool *threadPool);

ThreadPool* initializeThreadpool(int numberOfThreads, CircularBuffer* buffer, uint32_t ip, uint16_t port);

void *clientThread(void* arg);

Socket* initializeSocket(uint16_t port);

#endif //WHOCLIENT_WHOCLIENTIO_H
