//
// Created by AriannaK97 on 11/6/20.
//

#include <features.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>	     /* sockets */
#include <ctype.h>	         /* toupper */
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <netinet/in.h>
#include "../../header/serverIO.h"
#include "../../header/list_lib.h"
#include "../../header/command_lib.h"

ServerInputArgs* getWhoServerArguments(int argc, char** argv){
    ServerInputArgs* arguments = calloc(sizeof(ServerInputArgs), 1);
    int numOfArgs = 0;
    if(argc != 9){
        fprintf(stderr, "Invalid number of arguments\nExit...\n");
        exit(1);
    }
    for (int i = 1; i < argc; i += 2) {
        if (strcmp(argv[i], "-q") == 0) {
            arguments->queryPortNum = atoi(argv[i + 1]);
        } else if (strcmp(argv[i], "-s") == 0) {
            arguments->statisticsPortNum = atoi(argv[i + 1]);
        } else if (strcmp(argv[i], "-b") == 0) {
            arguments->bufferSize = atoi(argv[i + 1]);
            if(arguments->bufferSize < 120){
                arguments->bufferSize = 120;
            }
            numOfArgs += 2;
        }else if(strcmp(argv[i], "-w") == 0){
            arguments->numThreads = atoi(argv[i + 1]);
        }else {
            fprintf(stderr, "Unknown option %s\n", argv[i]);
            exit(1);
        }
    }
    return arguments;
}

WhoServerManager* initializeWhoServerManager(ServerInputArgs* serverInputArgs){

    whoServerManager = calloc(sizeof(WhoServerManager), 1);

    whoServerManager->statisticsPortNum = serverInputArgs->statisticsPortNum;
    pthread_mutex_init(&(whoServerManager->mtxW), NULL);
    whoServerManager->queryPortNum = serverInputArgs->queryPortNum;
    whoServerManager->numThreads = serverInputArgs->numThreads;
    whoServerManager->bufferSize = serverInputArgs->bufferSize;
    whoServerManager->clientArrayIndex = 0;
    whoServerManager->numOfClients = 0;
    whoServerManager->workerArrayIndex = 0;
    whoServerManager->waitClient = false;
    whoServerManager->numOfWorkers = -1;
    whoServerManager->numOfWorkersEnd = 0;

    return whoServerManager;
}


/* Wait for all dead child processes */
void sigchld_handler (int sig){
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

void perror_exit(char *message){
    perror(message);
    exit(EXIT_FAILURE);
}


bool receiveStats(int readBufferSize, int sock){
    char *fileName, *disease, *message, *messageSize;
    int numOfDirs, numOfFiles, numOfDiseases;
    Node *listNode;
    CountryListItem *countryListItem;

    /*read per country*/
    message = calloc(sizeof(char), readBufferSize+1);
    if (read(sock, message, readBufferSize+1) < 0) {
        perror("problem in reading");
        exit(5);
    }
    numOfDirs = atoi(message);
    free(message);

    for (int i = 0; i < numOfDirs; i++) {
        /*read actual message from fifo*/
        countryListItem = calloc(sizeof(CountryListItem), 1);
        countryListItem->country = calloc(sizeof(char), readBufferSize+1);
        read(sock, countryListItem->country,readBufferSize+1);
        listNode = nodeInit(countryListItem->country);

        /*critical section for worker struct*/
        fprintf(stderr, "MtXlock\n");
        pthread_mutex_lock(&(whoServerManager->mtxW));

        if(whoServerManager->workerItemArray[whoServerManager->workerArrayIndex].list == NULL)
            whoServerManager->workerItemArray[whoServerManager->workerArrayIndex].list = linkedListInit(listNode);
        else
            push(listNode, whoServerManager->workerItemArray[whoServerManager->workerArrayIndex].list);

        pthread_mutex_unlock(&(whoServerManager->mtxW));
        fprintf(stderr, "MtX-Un-lock\n");

        /*read per file*/
        messageSize = calloc(sizeof(char), readBufferSize+1);
        read(sock, messageSize, readBufferSize+1);
        numOfFiles = atoi(messageSize);
        free(messageSize);
        for (int j = 0; j < numOfFiles; j++) {
            /*read actual message from fifo*/
            fileName = calloc(sizeof(char), readBufferSize+1);
            read(sock, fileName,readBufferSize+1);
            //fprintf(stdout, "\n%s\n%s\n",fileName, countryListItem->country);

            /*read per disease*/
            messageSize = calloc(sizeof(char), readBufferSize+1);
            read(sock, messageSize, readBufferSize+1);
            numOfDiseases = atoi(messageSize);
            free(messageSize);
            for (int k = 0; k < numOfDiseases; k++) {
                /*read actual message from fifo*/
                disease = calloc(sizeof(char), readBufferSize+1);
                read(sock, disease,readBufferSize+1);
                //fprintf(stdout, "%s\n", disease);

                for (int l = 0; l < 4; l++) {
                    /*read actual message from fifo*/
                    message = calloc(sizeof(char), readBufferSize+1);
                    read(sock, message,readBufferSize+1);
                    //fprintf(stdout, "%s\n", message);
                    free(message);
                }

                /*read actual message from fifo*/
                message = calloc(sizeof(char), readBufferSize+1);
                read(sock, message,readBufferSize+1);
                free(message);
                free(disease);
            }
            free(fileName);
        }
    }
    message = calloc(sizeof(char), readBufferSize+1);
    read(sock, message,readBufferSize+1);
    free(message);
    return true;
}



ThreadPool* initializeThreadpool(int numberOfThreads, CircularBuffer* buffer, uint32_t ip, uint16_t port){
    // Int new threadpool:
    ThreadPool* newThreadPool = malloc(sizeof(ThreadPool));
    newThreadPool->numberOfThreads = numberOfThreads;
    newThreadPool->threads = malloc(numberOfThreads * sizeof(pthread_t));
    newThreadPool->end = 0;
    newThreadPool->circularBuffer = buffer;
    newThreadPool->localIP = ip;
    newThreadPool->localPort = port;

    // Init mutex:
    pthread_mutex_init(&(newThreadPool->mutexLock), NULL);
    pthread_cond_init(&(newThreadPool->mutexCond), NULL);
    pthread_cond_init(&(newThreadPool->mutexCond2), NULL);

    // Create threads in threadpool:
    for(int i=0; i<numberOfThreads; i++){
        pthread_create(&(newThreadPool->threads[i]), NULL, workerThread, (void*)newThreadPool);
    }

    return newThreadPool;
}


// Initialize a socket with given port and ip:
Socket* initializeSocket(uint16_t port){
    Socket* newSocket = calloc(sizeof(Socket), 1);

    if((newSocket->socket = socket(AF_INET , SOCK_STREAM , 0)) < 0){
        perror("Error creating socket.");
        exit(EXIT_FAILURE);
    }

    // Set socket options:
    newSocket->socketAddressServer.sin_family = AF_INET;
    newSocket->socketAddressServer.sin_addr.s_addr = htonl(INADDR_ANY);
    newSocket->socketAddressServer.sin_port = htons(port);
    return newSocket;
}


void *workerThread(void* arg){

    ThreadPool* threadPool = (ThreadPool*)arg;
    int readWorkers = 0;
    int readClients = 0;
    int workerPort;
    int numOfWorkers, numOfClients;
    char* message;

    while (threadPool->end == 0){

        // Try to get lock:
        fprintf(stderr, "Mutexlock\n");
        pthread_mutex_lock(&(threadPool->mutexLock));

        // Wait until round buffer has available items:
        while(circularBufSize(threadPool->circularBuffer) <= 0){
            fprintf(stdout, "Thread waiting\n");
            pthread_cond_wait(&(threadPool->mutexCond), &(threadPool->mutexLock));
        }
        fprintf ( stdout, "Thread % ld : Woke up \n" , pthread_self () ) ;

        // If threadpool dead
        if(threadPool->end == 1){
            pthread_mutex_unlock(&(threadPool->mutexLock));
            fprintf(stderr, "Mutex-Un-lock - dead thread\n");
            break;
        }

        /**
         * Get a FileDescriptor item from the circular buffer
         * */
        int newSock;
        FileDescriptor fileDescriptor;
        circularBufGet(threadPool->circularBuffer, &fileDescriptor);
        newSock = fileDescriptor.fd;
        pthread_mutex_unlock(&(threadPool->mutexLock));
        pthread_cond_signal (&(threadPool->mutexCond));
        fprintf(stderr, "Mutex-Un-lock\n");

        /**
         * The socket comes from the worker thus we proceed to receiving the
         * statistics and the data needed from this connection
         */

        if(fileDescriptor.type == WORKER_SOCKET) {

            message = calloc(sizeof(char *), MESSAGE_BUFFER);
            read(newSock, message, MESSAGE_BUFFER);
            workerPort = atoi(message);
            //printf("--------------------------%d port\n\n", workerPort);

            flockfile(stdout);

            fprintf(stdout, "Worker port: %s\n", message);
            free(message);

            /*Get num of workers and initialize structures*/
            fprintf(stderr, "MtXlock\n");
            pthread_mutex_lock(&(whoServerManager->mtxW));

            message = calloc(sizeof(char *), MESSAGE_BUFFER);
            read(newSock, message, MESSAGE_BUFFER);
            numOfWorkers = atoi(message);
            free(message);

            if (readWorkers == 0) {
                whoServerManager->numOfWorkers = numOfWorkers;
                //printf("--------------------------%d workers\n\n", numOfWorkers);
                whoServerManager->workerItemArray = calloc(sizeof(WorkerItem), whoServerManager->numOfWorkers);
                readWorkers = whoServerManager->numOfWorkers;
                whoServerManager->workerArrayIndex = 0;
            }

            whoServerManager->workerItemArray[whoServerManager->workerArrayIndex].workerPort = workerPort;

            pthread_mutex_unlock(&(whoServerManager->mtxW));
            fprintf(stderr, "MtX-Un-lock\n");

            fprintf(stdout, "Receiving Statistics...\n");
            if (!receiveStats(MESSAGE_BUFFER, newSock)) {
                fprintf(stderr, "Could not receive statistics\n");
            }

            funlockfile(stdout);

            fprintf(stderr, "=>MtXlock\n");
            pthread_mutex_lock(&(whoServerManager->mtxW));
            whoServerManager->numOfWorkersEnd += 1;
            fprintf(stderr, "%d %d\n", whoServerManager->numOfWorkers ,whoServerManager->numOfWorkersEnd);
            if((whoServerManager->numOfWorkers) == (whoServerManager->numOfWorkersEnd)){
                fprintf(stderr, "Wait the clients\n");
                whoServerManager->waitClient = true;
            }
            pthread_mutex_unlock(&(whoServerManager->mtxW));
            fprintf(stderr, "=>MtX-Un-lock\n");

        } else if(fileDescriptor.type == CLIENT_SOCKET){
            flockfile(stdout);
            fprintf(stderr, "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");

            //read the query
            message = calloc(sizeof(char *), MESSAGE_BUFFER);
            read(newSock, message, MESSAGE_BUFFER);
            fprintf(stderr, "---%s\n", message);
            funlockfile(stdout);

        }


    }
    return NULL;
}


void sendQueryToWorker(char *query, int socketFd){
    char* simpleCommand;
    char* command;
    char* tempQuery = calloc(sizeof(char), MESSAGE_BUFFER);
    struct sockaddr *serverptr;
    Socket *newSocket;

    strcpy(tempQuery, query);
    simpleCommand = strtok(tempQuery, "\n");

    if (strcmp(simpleCommand, "/help") == 0) {
        helpDesc();
    }else {

        command = strtok(simpleCommand, " ");

        if (strcmp(command, "/diseaseFrequency") == 0) {
            Date *date1;
            Date *date2;
            date1 = malloc(sizeof(struct Date));
            date2 = malloc(sizeof(struct Date));

            char *virusName = strtok(NULL, " ");   //virus
            char *arg2 = strtok(NULL, " ");   //date1
            char *arg3 = strtok(NULL, " ");   //date2
            char *country = strtok(NULL, " ");

            if (arg2 != NULL && arg3 != NULL) {
                date1->day = atoi(strtok(arg2, "-"));
                date1->month = atoi(strtok(NULL, "-"));
                date1->year = atoi(strtok(NULL, "-"));
                date2->day = atoi(strtok(arg3, "-"));
                date2->month = atoi(strtok(NULL, "-"));
                date2->year = atoi(strtok(NULL, "-"));

                if (country != NULL) {
                    for(int i = 0; i < whoServerManager->numOfWorkers; i++){
                        newSocket = initializeSocket(whoServerManager->workerItemArray[i].workerPort);

                        serverptr = (struct sockaddr*)&(newSocket->socketAddressServer);
                        if (connect(newSocket->socket, serverptr, sizeof(newSocket->socketAddressServer)) < 0){
                            perror("Connect server");
                            exit(1);
                        }
                        printf("Connecting to worker server port %d to send query\n", whoServerManager->workerItemArray[i].workerPort);
                        write(newSocket->socket, query, MESSAGE_BUFFER);
                    }
                } else{
                    for(int i = 0; i < whoServerManager->numOfWorkers; i++){
                        newSocket = initializeSocket(whoServerManager->workerItemArray[i].workerPort);

                        serverptr = (struct sockaddr*)&(newSocket->socketAddressServer);
                        if (connect(newSocket->socket, serverptr, sizeof(newSocket->socketAddressServer)) < 0){
                            perror("Connect server");
                            exit(1);
                        }
                        printf("Connecting to worker server port %d to send query\n", whoServerManager->workerItemArray[i].workerPort);
                        write(newSocket->socket, query, MESSAGE_BUFFER);
                    }
                }

                free(date1);
                free(date2);
            }

        }else if (strcmp(command, "/topk-AgeRanges") == 0) {


        } else if (strcmp(command, "/searchPatientRecord") == 0) {


        } else if (strcmp(command, "/numPatientAdmissions") == 0) {


        } else if (strcmp(command, "/numPatientDischarges") == 0) {

        }
    }

}

