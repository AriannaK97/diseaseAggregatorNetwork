//
// Created by AriannaK97 on 11/6/20.
//

#include <stdio.h>
#include <zconf.h>
#include <arpa/inet.h>
#include "../../header/whoClientIO.h"

int main (int argc, char** argv) {

    struct sockaddr_in server;
    struct in_addr myaddress;
    struct sockaddr *serverptr;
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

    //whoClientManager->clientSocket = calloc(sizeof(Socket), 1);

    if ((whoClientManager->sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("client socket");
        exit(1);
    }

    /* Find server address */
    inet_aton((const char*)whoClientManager->servIP, &myaddress);
    server.sin_family = AF_INET;       /* Internet domain */
    server.sin_addr = myaddress;
    server.sin_port = htons(whoClientManager->servPort);         /* Server port */

    /* Initiate connection for server*/
    serverptr = (struct sockaddr*)&server;
    if (connect(whoClientManager->sock, serverptr, sizeof(server)) < 0){
        perror("Connect server");
        exit(1);
    }
    printf("Connecting to client port %d socket %d\n", whoClientManager->servPort, whoClientManager->sock);
    pthread_cond_broadcast(&(threadPool->mutexCond));

    while(threadPool->end != whoClientManager->numThreads);

}

