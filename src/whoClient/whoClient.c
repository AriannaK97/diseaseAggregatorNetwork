//
// Created by AriannaK97 on 11/6/20.
//

#include <stdio.h>
#include <zconf.h>
#include <arpa/inet.h>
#include "../../header/whoClientIO.h"

int main (int argc, char** argv) {

    socklen_t clientlen;
    int newSock;
    struct in_addr myaddress;
/*****************************************************************************
 *                       Handling command line arguments                     *
 *****************************************************************************/
    WhoClientInputArguments *arguments = getWhoClientArguments(argc, argv);

    whoClientManager = initializeWhoClientManager(arguments);

    freeWhoClientInputArgs(arguments);

    /*Create threadpool*/
    ThreadPool *threadPool = initializeThreadpool(whoClientManager->numThreads, NULL,
                                                  (uint32_t) whoClientManager->servIP, whoClientManager->servPort);

    readQueryFile(threadPool);

    /* Create socket for queries, then bind it to address and start listening to it*/

    whoClientManager->clientSocket = calloc(sizeof(Socket), 1);

    if ((whoClientManager->clientSocket->socket = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("client socket");
        exit(1);
    }

    /* Find server address */
    inet_aton((const char*)whoClientManager->servIP, &myaddress);
    whoClientManager->clientSocket->socketAddressServer.sin_family = AF_INET;       /* Internet domain */
    whoClientManager->clientSocket->socketAddressServer.sin_addr = myaddress;
    whoClientManager->clientSocket->socketAddressServer.sin_port = htons(whoClientManager->servPort);         /* Server port */

    /* Initiate connection for server*/
    struct sockaddr *serverptr = (struct sockaddr*)&(whoClientManager->clientSocket->socketAddressServer);
    if (connect(whoClientManager->clientSocket->socket, serverptr,
            sizeof(whoClientManager->clientSocket->socketAddressServer)) < 0){
        perror("Connect server");
        exit(1);
    }
    printf("Connecting to %s client port %d socket %d\n", argv[1], whoClientManager->servPort, whoClientManager->clientSocket->socket);
    pthread_cond_broadcast(&(threadPool->mutexCond));

    while(threadPool->end != whoClientManager->numThreads);
}

