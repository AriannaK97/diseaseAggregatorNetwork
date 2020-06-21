//
// Created by AriannaK97 on 11/6/20.
//

#ifndef WHOSERVERPROGRAM_SERVERIO_H
#define WHOSERVERPROGRAM_SERVERIO_H

#include <stdbool.h>
#include <pthread.h>
#include <netinet/in.h>
#include "structs.h"
#include "whoServerCircularBuffer.h"

#define MESSAGE_BUFFER 224

/**
 * Structures
 * */

typedef struct ServerInputArgs{
    uint16_t queryPortNum;
    uint16_t statisticsPortNum;
    int numThreads;
    int bufferSize;
}ServerInputArgs;

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
    int socketSize;
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
    Socket *serverSocket;
    Socket *clientSocket;
    int numOfWorkers;
    int numOfWorkersEnd;
    bool waitClient;
    int numOfClients;
    WorkerItem *workerItemArray;
    int workerArrayIndex;
    int clientArrayIndex;
    pthread_mutex_t mtxW;
}WhoServerManager;

typedef struct CountryListItem{
    char *country;
}CountryListItem;

enum Type{
    WORKER_SOCKET,
    CLIENT_SOCKET
};

/**
 * Global variables
 * */
WhoServerManager *whoServerManager;


/**
 * Method Declaration
 * */
ServerInputArgs* getWhoServerArguments(int argc, char** argv);

WhoServerManager* initializeWhoServerManager(ServerInputArgs* serverInputArgs);

void sigchld_handler (int sig);

void perror_exit(char *message);

bool receiveStats(int readBufferSize,int sock);

ThreadPool* initializeThreadpool(int numberOfThreads, CircularBuffer* buffer, uint32_t ip, uint16_t port);

Socket* initializeSocket(uint16_t port);

void * workerThread(void* arg);

void sendQueryToWorker(char *query, int socketFd);

bool workerHasCountry(char *country, struct List *workerCountryList);

bool compareListItemCountry(CountryListItem* countryItem, char* key);

void printCountryList(struct List *countryList);

#endif //WHOSERVERPROGRAM_SERVERIO_H
