//
// Created by AriannaK97 on 22/5/20.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "../../header/data_io.h"
#include "../../header/command_lib.h"
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
    char buf[256];
    //CmdManager* cmdManager;

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

    fprintf(stdout, "Worker has started\n");

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


    /*receive from master the length of data the worker will receive*/

    dataLengthStr = calloc(sizeof(char), (cmdManager->bufferSize)+1);
    readFromFifoPipe(cmdManager->fd_client_r, dataLengthStr, (cmdManager->bufferSize) + 1);
    dataLength = atoi(dataLengthStr);
    free(dataLengthStr);
    printf("datalength %d", dataLength);

    cmdManager->numOfDirectories = dataLength;
    int ghostBugResolver = dataLength;
    while(ghostBugResolver > 0){

    /*read actual message from fifo*/


        message = calloc(sizeof(char),(arguments->bufferSize)+1);
        readFromFifoPipe(cmdManager->fd_client_r, message,(arguments->bufferSize) + 1);
        messageSize = arguments->bufferSize;


        newNodeItem = (DirListItem*)malloc(sizeof(struct DirListItem));
        newNodeItem->dirName = (char*)malloc(sizeof(char)*DIR_LEN);
        newNodeItem->dirPath = (char*)malloc(sizeof(char)*DIR_LEN);

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

    int sock;
    struct sockaddr_in server;
    struct sockaddr *serverptr;
    struct in_addr myaddress;
    struct hostent *esu;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("socket");
        exit(1);
    }

    /* Find server address */
    inet_aton((const char*)cmdManager->serverIP, &myaddress);
    server.sin_family = AF_INET;       /* Internet domain */
    server.sin_addr = myaddress;
    server.sin_port = htons(cmdManager->serverPort);         /* Server port */
    /* Initiate connection */
    serverptr = (struct sockaddr*)&server;
    if (connect(sock, serverptr, sizeof(server)) < 0){
        perror("connect");
        exit(1);
    }
    printf("Connecting to %s port %d\n", argv[1], cmdManager->serverPort);

    do{
        strcpy(buf, "END\n");
        message = (char*)calloc(sizeof(char),DIR_LEN);
        sprintf(message, "%d", cmdManager->serverPort);
        write(sock, message, cmdManager->bufferSize);
        free(message);

        if(!sendStatistics(sock)){
            fprintf(stderr, "Could not send statistics\n");
        }

    }while(strcmp(buf, "END\n")!=0);

    close(sock);

    commandServer(cmdManager);

    /*for debug reasons*/
/*    cmdManager->numOfDirectories = 1;
    newNodeItem = (DirListItem*)malloc(sizeof(struct DirListItem));
    newNodeItem->dirName = (char*)malloc(sizeof(char)*DIR_LEN);
    newNodeItem->dirPath = (char*)malloc(sizeof(char)*DIR_LEN);

    memcpy(newNodeItem->dirName, "Australia", 256);
    strcpy(newNodeItem->dirPath, "/home/linuxuser/CLionProjects/DiseaseAggregator/input_dir/Australia");

    printf("dir path : %s\n", newNodeItem->dirPath);

    newNode = nodeInit(newNodeItem);
    if(cmdManager->directoryList == NULL){
        cmdManager->directoryList = linkedListInit(newNode);
    } else{
        push(newNode, cmdManager->directoryList);
    }
*/

    fprintf(stdout, "exiting child\n");
    close(cmdManager->fd_client_r);
    close(cmdManager->fd_client_w);
    exit(0);
}