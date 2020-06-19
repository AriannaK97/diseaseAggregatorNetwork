//
// Created by AriannaK97 on 11/6/20.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include "../../header/serverIO.h"


int main (int argc, char** argv){

    ServerInputArgs* serverInputArgs;
    int newSock, newSockClient;
    socklen_t clientlen, workerlen;

    serverInputArgs = getWhoServerArguments(argc, argv);
    whoServerManager = initializeWhoServerManager(serverInputArgs);
    free(serverInputArgs);

    /*Create and initialize circular buffer*/
    CircularBuffer *cBuffer = circularBufInit(whoServerManager->bufferSize);

    /*Create threadpool*/
    ThreadPool* threadPool = initializeThreadpool(whoServerManager->numThreads, cBuffer, 0, whoServerManager->statisticsPortNum);


    /* Create socket for statistics, then bind it to address and start listening to it*/
    whoServerManager->serverSocket = initializeSocket(whoServerManager->statisticsPortNum);
    whoServerManager->clientSocket = initializeSocket(whoServerManager->queryPortNum);

    if (bind(whoServerManager->serverSocket->socket, (struct sockaddr *)&(whoServerManager->serverSocket->socketAddressServer), sizeof(whoServerManager->serverSocket->socketAddressServer)) < 0)
        perror_exit("bind");

    if (listen(whoServerManager->serverSocket->socket, whoServerManager->numThreads) < 0)
        perror_exit("listen");
    printf("Listening for connections to port %d\n", whoServerManager->statisticsPortNum);


    while (1) {
        /* accept connection */
        workerlen = sizeof(whoServerManager->serverSocket->socketAddressClient);
        if ((newSock = accept((whoServerManager->serverSocket->socket), (struct sockaddr *)&(whoServerManager->serverSocket->socketAddressClient), &workerlen)) < 0)
            perror_exit("accept");

        workerlen = sizeof(whoServerManager->clientSocket->socketAddressClient);
        if((newSockClient = accept((whoServerManager->clientSocket->socket), (struct sockaddr *)&(whoServerManager->clientSocket->socketAddressClient), &clientlen) < 0))
            perror_exit("accept client");

        fprintf(stdout,"WhoServer accepted connection to worker\n");

        pthread_mutex_lock(&(threadPool->mutexLock));
        fprintf(stdout,"\n------------------------\n");
        fprintf(stdout,"\nGained lock to add item.\n");
        circularBufPut(threadPool->circularBuffer, newSock, WORKER_SOCKET);
        pthread_mutex_unlock(&(threadPool->mutexLock));
        pthread_cond_signal (&(threadPool->mutexCond));

        //close(newSock);
        /* must be closed before it gets re-assigned */
    }
}

