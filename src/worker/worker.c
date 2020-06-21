//
// Created by AriannaK97 on 22/5/20.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../../header/data_io.h"
#include "../../header/diseaseAggregator.h"
#include "../../header/communication.h"
#include "../../header/serverIO.h"

int main(int argc, char** argv) {

    int messageSize;
    char* message;
    int dataLength;
    char* dataLengthStr;
    DirListItem* newNodeItem = NULL;
    Node* newNode = NULL;

/*****************************************************************************
 *                       Handling command line arguments                     *
 *****************************************************************************/
    MonitorInputArguments* arguments = getMonitorInputArgs(argc, argv);

/*****************************************************************************
 *                            Signal Handling                                *
 *****************************************************************************/

    signal(SIGINT, sigintHandler);
    signal(SIGQUIT, sigintHandler);
    signal(SIGUSR1, checkForNewFilesInSubDirs_handler);


/*****************************************************************************
 *                       Handling input files                                *
 *****************************************************************************/

    cmdManager = initializeStructures(arguments);
    cmdManager->workerInfo->workerPid = getpid();
    cmdManager->workerId = arguments->workerId;

    /*create endpoint of fifo from master*/

    make_fifo_name_server_client(cmdManager->workerInfo->workerPid, cmdManager->workerInfo->serverFileName);
    createNewFifoPipe(cmdManager->workerInfo->serverFileName);

    cmdManager->fd_client_r = openFifoToRead(cmdManager->workerInfo->serverFileName);

    /*receive serverIP*/
    message = calloc(sizeof(char),(arguments->bufferSize)+1);
    readFromFifoPipe(cmdManager->fd_client_r, message,(arguments->bufferSize) + 1);
    cmdManager->serverIP = message;
    free(message);

    /*receive serverPort*/
    message = calloc(sizeof(char),(arguments->bufferSize)+1);
    readFromFifoPipe(cmdManager->fd_client_r, message,(arguments->bufferSize) + 1);
    cmdManager->serverPort = atoi(message);
    free(message);

    /*receive numOfWorkers in system*/
    message = calloc(sizeof(char),(arguments->bufferSize)+1);
    readFromFifoPipe(cmdManager->fd_client_r, message,(arguments->bufferSize) + 1);
    cmdManager->numOfWorkers = atoi(message);
    free(message);

    /*receive from master the length of data the worker will receive*/
    dataLengthStr = calloc(sizeof(char), (cmdManager->bufferSize)+1);
    readFromFifoPipe(cmdManager->fd_client_r, dataLengthStr, (cmdManager->bufferSize) + 1);
    dataLength = atoi(dataLengthStr);
    free(dataLengthStr);

    cmdManager->numOfDirectories = dataLength;
    int ghostBugResolver = dataLength;
    while(ghostBugResolver > 0){

    /*read actual message from fifo*/

        message = calloc(sizeof(char),(arguments->bufferSize)+1);
        readFromFifoPipe(cmdManager->fd_client_r, message,(arguments->bufferSize) + 1);
        messageSize = arguments->bufferSize;


        newNodeItem = (DirListItem*)malloc(sizeof(struct DirListItem));
        newNodeItem->dirName = (char*)malloc(sizeof(char)*MESSAGE_SIZE);
        newNodeItem->dirPath = (char*)malloc(sizeof(char)*MESSAGE_SIZE);

        memcpy(newNodeItem->dirName, message, messageSize+1);
        strcpy(newNodeItem->dirPath, arguments->input_dir);
        strcat(newNodeItem->dirPath, "/");
        strcat(newNodeItem->dirPath, message);

        newNode = nodeInit(newNodeItem);
        if(cmdManager->directoryList == NULL){
            cmdManager->directoryList = linkedListInit(newNode);
        } else{
            push(newNode, cmdManager->directoryList);
        }

        //printf("%s\n", newNodeItem->dirName);
        ghostBugResolver = ghostBugResolver - 1;
        free(message);

    }

    cmdManager = read_directory_list(cmdManager);

    /**
     * Connect to server and start communication
     * */

    struct sockaddr_in server, client;
    struct sockaddr *serverptr;
    struct in_addr myaddress;

    if ((cmdManager->sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Server socket");
        exit(1);
    }

    if ((cmdManager->workerSock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Worker socket");
        exit(1);
    }

    /* Find server address */
    inet_aton((const char*)cmdManager->serverIP, &myaddress);
    server.sin_family = AF_INET;       /* Internet domain */
    server.sin_addr = myaddress;
    server.sin_port = htons(cmdManager->serverPort);         /* Server port */

    /*Worker address assign from system*/
    client.sin_family = AF_INET;
    client.sin_addr = myaddress;
    client.sin_port = 0;

    /* Initiate connection for server*/
    serverptr = (struct sockaddr*)&server;
    if (connect(cmdManager->sock, serverptr, sizeof(server)) < 0){
        perror("Connect server");
        exit(1);
    }
    printf("Connecting to %s server port %d\n", argv[1], cmdManager->serverPort);

    /* Bind socket to address */
    if (bind(cmdManager->workerSock, (struct sockaddr *)&client, sizeof(client)) < 0){
        perror("Bind");
        exit(1);
    }

    socklen_t socklen = sizeof(client);
    if(getsockname(cmdManager->workerSock, (struct sockaddr *)&client, &socklen) == -1){
        perror("getsockname");
        exit(1);
    }


    cmdManager->workerPort = ntohs(client.sin_port);
    /*send port*/
    message = (char*)calloc(sizeof(char),MESSAGE_BUFFER);
    sprintf(message, "%d", cmdManager->workerPort);
    write(cmdManager->sock, message, MESSAGE_BUFFER);
    free(message);

    /*send num of workers*/
    message = (char*)calloc(sizeof(char),MESSAGE_BUFFER);
    sprintf(message, "%d", cmdManager->numOfWorkers);
    write(cmdManager->sock, message, MESSAGE_BUFFER);
    free(message);

    if(!sendStatistics(cmdManager->sock)){
        fprintf(stderr, "Could not send statistics\n");
    }

    /* Initiate connection for worker port*/
    cmdManager->workerptr = (struct sockaddr*)&client;
    if (listen(cmdManager->workerSock, 100) < 0){
        perror("Listen worker");
        exit(1);
    }
    printf("Listening for connections to worker port %d\n", cmdManager->workerPort);

    commandServer(cmdManager);

    close(cmdManager->sock);
    fprintf(stdout, "exiting child\n");
    exit(0);
}