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

    whoClientManager->serverSocket = calloc(sizeof(Socket), 1);
    whoClientManager->clientSocket = calloc(sizeof(Socket), 1);

    if ((whoClientManager->serverSocket->socket = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Server socket");
        exit(1);
    }

    if ((whoClientManager->clientSocket->socket = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("client socket");
        exit(1);
    }

    /* Find server address */
    inet_aton((const char*)whoClientManager->servIP, &myaddress);
    whoClientManager->serverSocket->socketAddressServer.sin_family = AF_INET;       /* Internet domain */
    whoClientManager->serverSocket->socketAddressServer.sin_addr = myaddress;
    whoClientManager->serverSocket->socketAddressServer.sin_port = htons(whoClientManager->servPort);         /* Server port */

    /*Worker address assign from system*/
    whoClientManager->clientSocket->socketAddressServer.sin_family = AF_INET;
    whoClientManager->clientSocket->socketAddressServer.sin_addr = myaddress;
    whoClientManager->clientSocket->socketAddressServer.sin_port = 0;

    /* Initiate connection for server*/
    struct sockaddr *serverptr = (struct sockaddr*)&(whoClientManager->serverSocket->socketAddressServer);
    if (connect(whoClientManager->serverSocket->socket, serverptr, sizeof(whoClientManager->serverSocket->socketAddressServer)) < 0){
        perror("Connect server");
        exit(1);
    }
    printf("Connecting to %s server port %d\n", argv[1], whoClientManager->servPort);

    if (bind(whoClientManager->clientSocket->socket, (struct sockaddr *)&(whoClientManager->clientSocket->socketAddressServer), sizeof(whoClientManager->clientSocket->socketAddressServer)) < 0){
        perror("bind");
        exit(EXIT_FAILURE);
    }

    socklen_t socklen;
    if(getsockname(whoClientManager->clientSocket->socket, (struct sockaddr *)&(whoClientManager->clientSocket->socketAddressClient), &socklen) == -1){
        perror("getsockname");
        exit(1);
    }

    while(threadPool->end != whoClientManager->numThreads);
}

