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
    int newSock;
    socklen_t clientlen;

    serverInputArgs = getWhoServerArguments(argc, argv);
    whoServerManager = initializeWhoServerManager(serverInputArgs);
    free(serverInputArgs);

    /*Create and initialize circular buffer*/
    CircularBuffer *cBuffer = circularBufInit(whoServerManager->bufferSize);

    /*Create threadpool*/
    ThreadPool* threadPool = initializeThreadpool(whoServerManager->numThreads, cBuffer, 0, whoServerManager->statisticsPortNum);


    /* Create socket for statistics, then bind it to address and start listening to it*/
    whoServerManager->sock = initializeSocket(whoServerManager->statisticsPortNum, WORKER_SOCKET);

    if (bind(whoServerManager->sock->socket, whoServerManager->sock->serverptr, sizeof(whoServerManager->sock->socketAddressServer)) < 0)
        perror_exit("bind");

    if (listen(whoServerManager->sock->socket, whoServerManager->numThreads) < 0)
        perror_exit("listen");
    printf("Listening for connections to port %d\n", whoServerManager->statisticsPortNum);


    while (1) {
        /* accept connection */
        if ((newSock = accept((whoServerManager->sock->socket), whoServerManager->sock->clientptr, &clientlen)) < 0)
            perror_exit("accept");

        printf("Accepted connection\n");

        pthread_mutex_lock(&(threadPool->mutexLock));
        fprintf(stdout,"\n------------------------\n");
        fprintf(stdout,"\nGained lock to add item.\n");
        circularBufPut(threadPool->circularBuffer, newSock);
        pthread_mutex_unlock(&(threadPool->mutexLock));
        pthread_cond_signal (&(threadPool->mutexCond));

        //close(newSock);
        /* must be closed before it gets re-assigned */
    }
}

