//
// Created by AriannaK97 on 11/6/20.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include "../../header/serverIO.h"
#include "../../header/diseaseAggregator.h"

int main (int argc, char** argv){

    ServerInputArgs* serverInputArgs;
    int newSock;
    struct sockaddr_in server, client;
    socklen_t clientlen;
    char* message;

    serverInputArgs = getWhoServerArguments(argc, argv);
    whoServerManager = initializeWhoServerManager(serverInputArgs);
    free(serverInputArgs);

    /*Create and initialize circular buffer*/
    CircularBuffer *cBuffer = circularBufInit(whoServerManager->bufferSize);

    ThreadPool* threadPool = initializeThreadpool(whoServerManager->numThreads, cBuffer, 0, whoServerManager->statisticsPortNum);

    /* Create socket for statistics*/
    whoServerManager->sock = initializeSocket(whoServerManager->statisticsPortNum, 0, WORKER_SOCKET);

    /* Bind socket to address */
    if (bind(whoServerManager->sock->socket, whoServerManager->sock->serverptr, sizeof(whoServerManager->sock->socketAddressServer)) < 0)
        perror_exit("bind");

    /* Listen for connections */
    if (listen(whoServerManager->sock->socket, whoServerManager->numThreads) < 0)
        perror_exit("listen");
    printf("Listening for connections to port %d\n", whoServerManager->statisticsPortNum);

    while (1) {
        /* accept connection */
        if ((newSock = accept((whoServerManager->sock->socket), whoServerManager->sock->clientptr, &clientlen)) < 0)
            perror_exit("accept");

        printf("Accepted connection\n");

        pthread_mutex_lock(&(threadPool->mutexLock));
        printf("\n  Gained lock to add item.\n");
        circularBufPut(threadPool->circularBuffer, newSock);
        printf("%zu\n", circularBufSize(threadPool->circularBuffer));
        pthread_mutex_unlock(&(threadPool->mutexLock));
        pthread_cond_signal (&(threadPool->mutexCond));

        //close(newSock);
        /* must be closed before it gets re-assigned */
    }



}

