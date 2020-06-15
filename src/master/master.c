#define POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include "../../header/diseaseAggregator.h"
#include "../../header/communication.h"

void receiveStatsUpdate(int signo, siginfo_t *si, void *data);

int main(int argc, char** argv){
    Node* currentNode;
    DirListItem* item;
    char *message;
    pid_t pid;
/*****************************************************************************
 *                       Handling command line arguments                     *
 *****************************************************************************/

    AggregatorInputArguments* arguments = getAggregatorInputArgs(argc, argv);

/*****************************************************************************
 *                 Equal distribution of files between workers               *
 *****************************************************************************/

    aggregatorMasterManager = readDirectoryFiles(arguments);
    //printAggregatorManagerDirectoryDistributor(DiseaseAggregatorServerManager, arguments->numWorkers);

/*****************************************************************************
 *                            Signal Handling                                *
 *****************************************************************************/
    signal(SIGCHLD, respawnWorker);
    signal(SIGINT, aggregatorLogFile);
    signal(SIGQUIT, aggregatorLogFile);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = receiveStatsUpdate;
    sigaction(SIGUSR1, &sa, NULL);


/*****************************************************************************
 *                               Start Server                                *
 *****************************************************************************/

    fprintf(stdout,"master has started...\n");

    aggregatorMasterManager->bufferSize = arguments->bufferSize;
    aggregatorMasterManager->numOfWorkers = arguments->numWorkers;
    aggregatorMasterManager->input_dir = calloc(sizeof(char), DIR_LEN);
    aggregatorMasterManager->serverIP = calloc(sizeof(char), aggregatorMasterManager->bufferSize);
    strcpy(aggregatorMasterManager->serverIP, arguments->serverIP);
    aggregatorMasterManager->serverPort = arguments->serverPort;
    strcpy(aggregatorMasterManager->input_dir, arguments->input_dir);
    aggregatorMasterManager->workersArray = (WorkerInfo*)malloc(sizeof(WorkerInfo) * aggregatorMasterManager->numOfWorkers);

    freeAggregatorInputArguments(arguments);

    /*create workers*/
    for(int i = 0; i < aggregatorMasterManager->numOfWorkers; i++){

        if ((pid = fork()) == -1) {
            perror("fork error");
            exit(1);
        }else if (pid == 0) {
            char* bufferSize_str = (char*)malloc(sizeof(char)*(aggregatorMasterManager->bufferSize) + 1);
            char* workerId_str = (char*)malloc(sizeof(char)*(aggregatorMasterManager->bufferSize) + 1);
            sprintf(bufferSize_str, "%zu", aggregatorMasterManager->bufferSize);
            sprintf(workerId_str, "%d", i);
            execlp("./diseaseMonitor_worker", "./diseaseMonitor_worker", bufferSize_str, "5", "5" , "256",
                   aggregatorMasterManager->input_dir, workerId_str, (char*)NULL);
            fprintf(stdout,"Return not expected. Must be an execv error.n\n");
            free(bufferSize_str);
        }

        aggregatorMasterManager->workersArray[i].workerPid = pid;
        aggregatorMasterManager->workersArray[i].serverFileName = (char*)malloc(sizeof(char) * DIR_LEN);
        aggregatorMasterManager->workersArray[i].workerFileName = (char*)malloc(sizeof(char) * DIR_LEN);


        /*make fifo pipe for master*/
        make_fifo_name_server_client(pid, aggregatorMasterManager->workersArray[i].serverFileName);
        createNewFifoPipe(aggregatorMasterManager->workersArray[i].serverFileName);
        aggregatorMasterManager->workersArray[i].fd_client_w = openFifoToWrite(aggregatorMasterManager->workersArray[i].serverFileName);

        /*send serverIP address*/
        writeInFifoPipe(aggregatorMasterManager->workersArray[i].fd_client_w, aggregatorMasterManager->serverIP, (aggregatorMasterManager->bufferSize) + 1);

        /*send serverPort address*/
        message = calloc(sizeof(char), aggregatorMasterManager->bufferSize + 1);
        sprintf(message, "%d", aggregatorMasterManager->serverPort);
        writeInFifoPipe(aggregatorMasterManager->workersArray[i].fd_client_w, message, (aggregatorMasterManager->bufferSize) + 1);
        free(message);

        /*send the length of the data the worker has to read*/
        message = calloc(sizeof(char), aggregatorMasterManager->bufferSize + 1);
        sprintf(message, "%d", aggregatorMasterManager->directoryDistributor[i]->itemCount);
        writeInFifoPipe(aggregatorMasterManager->workersArray[i].fd_client_w, message, (aggregatorMasterManager->bufferSize) + 1);
        free(message);

        currentNode = (Node*)aggregatorMasterManager->directoryDistributor[i]->head;
        while (currentNode != NULL){
            item = currentNode->item;
            /*write the size of the name of the directory to follow to fifo*/
            //printf("from master %d\n", messageSize);
            /*write the directory name to fifo*/
            message = calloc(sizeof(char), (aggregatorMasterManager->bufferSize) + 1);
            strcpy(message, item->dirName);
            writeInFifoPipe(aggregatorMasterManager->workersArray[i].fd_client_w, message, (size_t)(aggregatorMasterManager->bufferSize) + 1);
            free(message);

            currentNode = currentNode->next;
        }


        /*start receiving*/
/*        make_fifo_name_client_server(pid, aggregatorMasterManager->workersArray[i].workerFileName);
        createNewFifoPipe(aggregatorMasterManager->workersArray[i].workerFileName);
        aggregatorMasterManager->workersArray[i].fd_client_r = openFifoToRead(aggregatorMasterManager->workersArray[i].workerFileName);*/

        /*read actual message from fifo*/
/*        message = calloc(sizeof(char), (aggregatorMasterManager->bufferSize) + 1);
        readFromFifoPipe(aggregatorMasterManager->workersArray[i].fd_client_r, message, (aggregatorMasterManager->bufferSize) + 1);

        fprintf(stdout, "%s\n", message);

        if(!receiveStats(aggregatorMasterManager, i)){
            fprintf(stderr, "Could not receive statistics\n");
        }

        free(message);*/
    }

    //DiseaseAggregatorServerManager(aggregatorMasterManager);


    return 0;
}



