//
// Created by AriannaK97 on 11/6/20.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/time.h>
#include "../../header/serverIO.h"

#define SOCKET_NUM 2
#define TIMEOUT          (1024 * 1024)


int main (int argc, char** argv){

    ServerInputArgs* serverInputArgs;
    int newSock;
    int newSockClient = -1;
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

    if (bind(whoServerManager->serverSocket->socket, (struct sockaddr *)&(whoServerManager->serverSocket->socketAddressServer), sizeof(whoServerManager->serverSocket->socketAddressServer)) < 0)
        perror_exit("bind");

    if (listen(whoServerManager->serverSocket->socket, whoServerManager->numThreads * 2) < 0)
        perror_exit("listen");
    fprintf(stdout,"Listening for connections to port %d\n", whoServerManager->statisticsPortNum);


    /* Create socket for queries, then bind it to address and start listening to it*/
    whoServerManager->clientSocket = initializeSocket(whoServerManager->queryPortNum);

    if (bind(whoServerManager->clientSocket->socket, (struct sockaddr *)&(whoServerManager->clientSocket->socketAddressServer), sizeof(whoServerManager->clientSocket->socketAddressServer)) < 0)
        perror_exit("bind");

    if (listen(whoServerManager->clientSocket->socket, whoServerManager->numThreads * 2) < 0)
        perror_exit("listen");
    fprintf(stdout,"Listening for connections to port %d\n", whoServerManager->queryPortNum);


    while (1){

        while ((whoServerManager->numOfWorkers) < (whoServerManager->numOfWorkersEnd)) {

            /* accept connections */

            workerlen = sizeof(whoServerManager->serverSocket->socketAddressClient);
            if ((newSock = accept((whoServerManager->serverSocket->socket),
                                  (struct sockaddr *) &(whoServerManager->serverSocket->socketAddressClient),
                                  &workerlen)) < 0)
                perror_exit("accept");


            flockfile(stdout);
            fprintf(stdout, "WhoServer accepted connection to worker\n");

            pthread_mutex_lock(&(threadPool->mutexLock));

            fprintf(stdout, "\nGained lock to add item.\n");
            circularBufPut(threadPool->circularBuffer, newSock, WORKER_SOCKET);

            pthread_mutex_unlock(&(threadPool->mutexLock));
            pthread_cond_signal(&(threadPool->mutexCond));

            funlockfile(stdout);

        }

        while(whoServerManager->waitClient==true){

            flockfile(stdout);
            fprintf(stderr, "\nWaiting queries from client\n");

            clientlen = sizeof(whoServerManager->clientSocket->socketAddressClient);
            while(1){
                newSockClient = accept((whoServerManager->clientSocket->socket),(struct sockaddr *) &(whoServerManager->clientSocket->socketAddressClient),&clientlen);
                if (newSockClient < 0)
                    perror_exit("accept client");
                else if(newSockClient == 0){
                    continue;
                }
                else {
                    fprintf(stdout, "WhoServer accepted connection to client %d\n", newSockClient);
                    break;
                }
            }

            pthread_mutex_lock(&(threadPool->mutexLock));

            fprintf(stderr, "\n------------------------\n");
            fprintf(stderr, "\nGained lock to add item of type client socket.\n");
            circularBufPut(threadPool->circularBuffer, newSockClient, CLIENT_SOCKET);
            funlockfile(stdout);
            pthread_mutex_unlock(&(threadPool->mutexLock));
            usleep(10000);
            pthread_cond_signal(&(threadPool->mutexCond));

        }

    }

}


