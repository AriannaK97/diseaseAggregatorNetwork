//
// Created by AriannaK97 on 22/5/20.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../../header/data_io.h"
#include "../../header/command_lib.h"
#include "../../header/diseaseAggregator.h"
#include "../../header/communication.h"

int main(int argc, char** argv) {

    int messageSize;
    char* message;
    int dataLength;
    char* dataLengthStr;
    DirListItem* newNodeItem = NULL;
    Node* newNode = NULL;
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



    cmdManager = initializeStructures(arguments);
    cmdManager->workerInfo->workerPid = getpid();
    cmdManager->workerId = arguments->workerId;

    /*create endpoint of fifo from server*/

    make_fifo_name_server_client(cmdManager->workerInfo->workerPid, cmdManager->workerInfo->serverFileName);
    createNewFifoPipe(cmdManager->workerInfo->serverFileName);

    cmdManager->fd_client_r = openFifoToRead(cmdManager->workerInfo->serverFileName);


    /*receive from server the length of data the client will receive*/

    dataLengthStr = calloc(sizeof(char), (cmdManager->bufferSize)+1);
    readFromFifoPipe(cmdManager->fd_client_r, dataLengthStr, (cmdManager->bufferSize) + 1);
    dataLength = atoi(dataLengthStr);
    free(dataLengthStr);

    cmdManager->numOfDirectories = dataLength;
    int ghostBugResolver = dataLength;
    while(ghostBugResolver > 0){

    /*read actual message from fifo*/


        message = malloc(sizeof(char)*(arguments->bufferSize)+1);
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

        ghostBugResolver = ghostBugResolver - 1;
        free(message);

    }

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


    cmdManager = read_directory_list(cmdManager);

    /**
     * Send success message back to parent through clients fifo
     * */
    make_fifo_name_client_server(cmdManager->workerInfo->workerPid, cmdManager->workerInfo->workerFileName);
    createNewFifoPipe(cmdManager->workerInfo->workerFileName);
    cmdManager->fd_client_w = openFifoToWrite(cmdManager->workerInfo->workerFileName);

    message = (char*)calloc(sizeof(char),DIR_LEN);
    strcpy(message, "Worker with pid has started...\n");
    /*write the message to fifo*/

    writeInFifoPipe(cmdManager->fd_client_w, message, (arguments->bufferSize)+1);

    if(!sendStatistics(cmdManager)){
        fprintf(stderr, "Could not send statistics\n");
    }

    fflush(stdout);
    free(arguments);

    commandServer(cmdManager);

    /**
     * Uncomment the line below to see all the inserted patients in the list
     * */

    //printList(cmdManager->patientList);


    /**
     * Uncomment the two lines below to see the hashtable contents
     * */
    //applyOperationOnHashTable(cmdManager->diseaseHashTable, PRINT);
    //applyOperationOnHashTable(cmdManager->countryHashTable, PRINT);

    fprintf(stdout, "exiting child\n");
    close(cmdManager->fd_client_r);
    close(cmdManager->fd_client_w);
    exit(0);
}