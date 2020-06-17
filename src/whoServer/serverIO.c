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
    whoServerManager->queryPortNum = serverInputArgs->queryPortNum;
    whoServerManager->numThreads = serverInputArgs->numThreads;
    whoServerManager->bufferSize = serverInputArgs->bufferSize;

    return whoServerManager;
}


void child_server(int newsock) {
    char buf[1];
    while(read(newsock, buf, 1) > 0) {  /* Receive 1 char */
        putchar(buf[0]);           /* Print received char */
        /* Capitalize character */
        buf[0] = toupper(buf[0]);
        /* Reply */
        if (write(newsock, buf, 1) < 0)
            perror_exit("write");
    }
    printf("Closing connection.\n");
    close(newsock);	  /* Close socket */
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
        countryListItem = calloc(sizeof(char), 1);
        countryListItem->country = calloc(sizeof(char), readBufferSize+1);
        read(sock, countryListItem->country,readBufferSize+1);
        listNode = nodeInit(countryListItem->country);

        /*critical section for worker struct*/
        pthread_mutex_lock(&(whoServerManager->mtxW));

        if(whoServerManager->workerItemArray[whoServerManager->workerArrayIndex].list == NULL)
            whoServerManager->workerItemArray[whoServerManager->workerArrayIndex].list = linkedListInit(listNode);
        else
            push(listNode, whoServerManager->workerItemArray[whoServerManager->workerArrayIndex].list);

        pthread_mutex_unlock(&(whoServerManager->mtxW));

        /*read per file*/
        messageSize = calloc(sizeof(char), readBufferSize+1);
        read(sock, messageSize, readBufferSize+1);
        numOfFiles = atoi(messageSize);
        free(messageSize);
        for (int j = 0; j < numOfFiles; j++) {
            /*read actual message from fifo*/
            fileName = calloc(sizeof(char), readBufferSize+1);
            read(sock, fileName,readBufferSize+1);
            fprintf(stdout, "\n%s\n%s\n",fileName, countryListItem->country);

            /*read per disease*/
            messageSize = calloc(sizeof(char), readBufferSize+1);
            read(sock, messageSize, readBufferSize+1);
            numOfDiseases = atoi(messageSize);
            free(messageSize);
            for (int k = 0; k < numOfDiseases; k++) {
                /*read actual message from fifo*/
                disease = calloc(sizeof(char), readBufferSize+1);
                read(sock, disease,readBufferSize+1);
                fprintf(stdout, "%s\n", disease);

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


CircularBuffer* circularBufInit(size_t bufferSize){
    CircularBuffer* cbuf = calloc(sizeof(CircularBuffer), 1);

    cbuf->buffer = calloc(sizeof(int), bufferSize);
    cbuf->max = bufferSize;
    circularBufReset(cbuf);

    return cbuf;
}

void circularBufFree(CircularBuffer *cbuf){
    free(cbuf);
}

void circularBufReset(CircularBuffer *cbuf){
    cbuf->head = 0;
    cbuf->tail = 0;
    cbuf->full = false;
}

void advancePointer(CircularBuffer *cbuf){
    if(cbuf->full){
        cbuf->tail = (cbuf->tail + 1) % cbuf->max;
    }
    cbuf->head = (cbuf->head + 1) % cbuf->max;
    cbuf->full = (cbuf->head == cbuf->tail);
}

void retreatPointer(CircularBuffer *cbuf){
    cbuf->full = false;
    cbuf->tail = (cbuf->tail + 1) % cbuf->max;
}

void circularBufPut(CircularBuffer *cbuf, int data){
    cbuf->buffer[cbuf->head] = data;
    advancePointer(cbuf);
}

int circularBufPut2(CircularBuffer *cbuf, int data){
    int r = -1;

    if(!circularBufFull(cbuf)){
        cbuf->buffer[cbuf->head] = data;
        advancePointer(cbuf);
        r = 0;
    }

    return r;
}

int circularBufGet(CircularBuffer *cbuf, int *data){

    int r = -1;

    if(!circularBufEmpty(cbuf)){
        *data = cbuf->buffer[cbuf->tail];
        retreatPointer(cbuf);

        r = 0;
    }

    return r;

}

bool circularBufEmpty(CircularBuffer *cbuf){
    return (!cbuf->full && (cbuf->head == cbuf->tail));
}

bool circularBufFull(CircularBuffer *cbuf){
    return cbuf->full;
}

size_t circularBufCapacity(CircularBuffer *cbuf){
    return cbuf->max;
}

size_t circularBufSize(CircularBuffer *cbuf){
    size_t size = cbuf->max;

    if(!cbuf->full){
        if(cbuf->head >= cbuf->tail){
            size = (cbuf->head - cbuf->tail);
        }else{
            size = (cbuf->max + cbuf->head - cbuf->tail);
        }
    }

    return size;
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
Socket* initializeSocket(uint16_t port, int type){
    Socket* newSocket = calloc(sizeof(Socket), 1);

    if((newSocket->socket = socket(AF_INET , SOCK_STREAM , 0)) < 0){
        perror("Error creating socket.");
        exit(EXIT_FAILURE);
    }

    /*set the socket type*/
    newSocket->type = type;

    newSocket->clientptr=(struct sockaddr *)&(newSocket->socketAddressClient);
    newSocket->serverptr=(struct sockaddr *)&(newSocket->socketAddressServer);

    // Set socket options:
    newSocket->socketAddressServer.sin_family = AF_INET;
    newSocket->socketAddressServer.sin_addr.s_addr = htonl(INADDR_ANY);
    newSocket->socketAddressServer.sin_port = htons(port);
    return newSocket;
}


void *workerThread(void* arg){

    ThreadPool* threadPool = (ThreadPool*)arg;
    int readWorkers = 0;
    int workerPort;
    int numOfWorkers;
    char* message;

    while (threadPool->end == 0){

        // Try to get lock:
        pthread_mutex_lock(&(threadPool->mutexLock));

        // Wait until round buffer has available items:
        while(circularBufSize(threadPool->circularBuffer) <= 0){
            pthread_cond_wait(&(threadPool->mutexCond), &(threadPool->mutexLock));
        }
        printf ( "Thread % ld : Woke up \n" , pthread_self () ) ;

        // If threadpool dead, get over it:
        if(threadPool->end == 1){
            pthread_mutex_unlock(&(threadPool->mutexLock));
            break;
        }

        // Get an item from round buffer:
        int newSock;
        circularBufGet(threadPool->circularBuffer, &newSock);
        pthread_mutex_unlock(&(threadPool->mutexLock));
        pthread_cond_signal (&(threadPool->mutexCond));

        flockfile(stdout);

        message = calloc(sizeof(char*), MESSAGE_BUFFER);
        read(newSock, message, MESSAGE_BUFFER);
        workerPort = atoi(message);
        fprintf(stdout, "Worker port: %s\n", message);
        free(message);

        message = calloc(sizeof(char*), MESSAGE_BUFFER);
        read(newSock, message, MESSAGE_BUFFER);
        numOfWorkers = atoi(message);
        free(message);

        /*Get num of workers and initialize structures*/
        pthread_mutex_lock(&(whoServerManager->mtxW));
        if(readWorkers == 0){
            whoServerManager->numOfWorkers = numOfWorkers;
            whoServerManager->workerItemArray = calloc(sizeof(WorkerItem), whoServerManager->numOfWorkers);
            readWorkers = whoServerManager->numOfWorkers;
            whoServerManager->workerArrayIndex = 0;
        }

        whoServerManager->workerItemArray[whoServerManager->workerArrayIndex].workerPort = workerPort;
        pthread_mutex_unlock(&(whoServerManager->mtxW));

        fprintf(stdout, "Receiving Statistics...\n");
        if(!receiveStats(MESSAGE_BUFFER, newSock)){
            fprintf(stderr, "Could not receive statistics\n");
        }

        whoServerManager->workerArrayIndex++;

        funlockfile(stdout);

    }
    return NULL;
}
