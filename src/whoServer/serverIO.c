//
// Created by AriannaK97 on 11/6/20.
//

#include <features.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>	     /* sockets */
#include <stdbool.h>
#include <stdint.h>
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
    char *fileName, *disease, *message, *messageSize, *country;
    int numOfDirs, numOfFiles, numOfDiseases;
    Node *listNode;
    CountryListItem *countryListItem;

    /*read per country*/
    message = calloc(sizeof(char), MESSAGE_BUFFER);
    if (read(sock, message, MESSAGE_BUFFER) < 0) {
        perror("problem in reading");
        exit(5);
    }
    numOfDirs = atoi(message);
    free(message);

    for (int i = 0; i < numOfDirs; i++) {
        /*read actual message from fifo*/
        countryListItem = calloc(sizeof(CountryListItem), 1);
        countryListItem->country = calloc(sizeof(char), MESSAGE_BUFFER);
        country = calloc(sizeof(char), MESSAGE_BUFFER);
        read(sock, country, MESSAGE_BUFFER);
        strcpy(countryListItem->country, country);
        listNode = nodeInit((void*)countryListItem);


        if((whoServerManager->workerItemArray[whoServerManager->workerArrayIndex].list) == NULL)
            whoServerManager->workerItemArray[whoServerManager->workerArrayIndex].list = linkedListInit(listNode);
        else
            push(listNode, (whoServerManager->workerItemArray[whoServerManager->workerArrayIndex].list));


        /*read per file*/
        messageSize = calloc(sizeof(char), MESSAGE_BUFFER);
        read(sock, messageSize, MESSAGE_BUFFER);
        numOfFiles = atoi(messageSize);
        free(messageSize);
        for (int j = 0; j < numOfFiles; j++) {
            /*read actual message from fifo*/
            fileName = calloc(sizeof(char), MESSAGE_BUFFER);
            read(sock, fileName,MESSAGE_BUFFER);
            fprintf(stdout, "\n%s\n%s\n",fileName, countryListItem->country);

            /*read per disease*/
            messageSize = calloc(sizeof(char), MESSAGE_BUFFER);
            read(sock, messageSize, MESSAGE_BUFFER);
            numOfDiseases = atoi(messageSize);
            free(messageSize);
            for (int k = 0; k < numOfDiseases; k++) {
                /*read actual message from fifo*/
                disease = calloc(sizeof(char), MESSAGE_BUFFER);
                read(sock, disease,MESSAGE_BUFFER);
                fprintf(stdout, "%s\n", disease);

                for (int l = 0; l < 4; l++) {
                    /*read actual message from fifo*/
                    message = calloc(sizeof(char), MESSAGE_BUFFER);
                    read(sock, message,MESSAGE_BUFFER);
                    fprintf(stdout, "%s\n", message);
                    free(message);
                }

                /*read actual message from fifo*/
                message = calloc(sizeof(char), MESSAGE_BUFFER);
                read(sock, message,MESSAGE_BUFFER);
                free(message);
                free(disease);
            }
            free(fileName);
        }
        free(country);
    }
    message = calloc(sizeof(char), MESSAGE_BUFFER);
    read(sock, message,MESSAGE_BUFFER);
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
    int workerPort;
    int numOfWorkers;
    char* message;

    while (threadPool->end == 0){

        // Try to get lock:
        pthread_mutex_lock(&(threadPool->mutexLock));

        // Wait until round buffer has available items:
        fprintf(stderr, "Thread waiting\n");
        pthread_cond_wait(&(threadPool->mutexCond), &(threadPool->mutexLock));
        fprintf (stderr, "Thread % ld : Woke up \n" , pthread_self () ) ;

        // If threadpool dead
        if(threadPool->end == 1){
            pthread_mutex_unlock(&(threadPool->mutexLock));
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

        /**
         * The socket comes from the worker thus we proceed to receiving the
         * statistics and the data needed from this connection
         */

        if(fileDescriptor.type == WORKER_SOCKET) {

            message = calloc(sizeof(char *), MESSAGE_BUFFER);
            read(newSock, message, MESSAGE_BUFFER);
            workerPort = atoi(message);
            free(message);

            /*Get num of workers and initialize structures*/
            pthread_mutex_lock(&(whoServerManager->mtxW));

            message = calloc(sizeof(char *), MESSAGE_BUFFER);
            read(newSock, message, MESSAGE_BUFFER);
            numOfWorkers = atoi(message);
            free(message);

            if (whoServerManager->numOfWorkers == -1) {
                whoServerManager->numOfWorkers = numOfWorkers;
                whoServerManager->workerItemArray = calloc(sizeof(WorkerItem), numOfWorkers);
                for (int i = 0; i < numOfWorkers; i++) {
                    whoServerManager->workerItemArray[i].list = NULL;
                }
                whoServerManager->workerArrayIndex = 0;
            }

            whoServerManager->workerItemArray[whoServerManager->workerArrayIndex].workerPort = workerPort;

            flockfile(stdout);
            fprintf(stdout, "\nReceiving Statistics...\n");
            if (!receiveStats(MESSAGE_BUFFER, newSock)) {
                fprintf(stderr, "Could not receive statistics\n");
            }
            funlockfile(stdout);

            whoServerManager->numOfWorkersEnd += 1;
            whoServerManager->workerArrayIndex += 1;
            //fprintf(stderr, "%d %d\n", whoServerManager->numOfWorkers ,whoServerManager->numOfWorkersEnd);
            if((whoServerManager->numOfWorkers) == (whoServerManager->numOfWorkersEnd)){
                whoServerManager->waitClient = true;
            }
            pthread_mutex_unlock(&(whoServerManager->mtxW));

        } else if(fileDescriptor.type == CLIENT_SOCKET){

            //read the query

            message = calloc(sizeof(char *), MESSAGE_BUFFER);

            read(newSock, message, MESSAGE_BUFFER);
            fprintf(stderr, "\nSending query: %s\n", message);

            //send the query to the worker/s responsible
            sendQueryToWorker(message, newSock);

            free(message);

            //funlockfile(stdout);
        }

    }
    return NULL;
}


void sendQueryToWorker(char *query, int socketFd){
    char* simpleCommand;
    char* command;
    char* tempQuery = calloc(sizeof(char), MESSAGE_BUFFER);


    strcpy(tempQuery, query);
    simpleCommand = strtok_r(tempQuery, "\n", &tempQuery);

    if (strcmp(simpleCommand, "/help") == 0) {
        helpDesc();
    }else {
        char* saveptr;

        command = strtok_r(simpleCommand, " ", &saveptr);

        if (strcmp(command, "/diseaseFrequency") == 0) {
            Date *date1;
            Date *date2;

            strtok_r(NULL, " ", &saveptr);   //virus
            char *arg2 = strtok_r(NULL, " ", &saveptr);   //date1
            char *arg3 = strtok_r(NULL, " ", &saveptr);   //date2
            char *country = strtok_r(NULL, " ", &saveptr);

            if (arg2 != NULL && arg3 != NULL) {

                date1 = malloc(sizeof(struct Date));
                date2 = malloc(sizeof(struct Date));

                date1->day = atoi(strtok_r(arg2, "-", &arg2));
                date1->month = atoi(strtok_r(NULL, "-", &arg2));
                date1->year = atoi(strtok_r(NULL, "-", &arg2));
                date2->day = atoi(strtok_r(arg3, "-", &arg2));
                date2->month = atoi(strtok_r(NULL, "-", &arg2));
                date2->year = atoi(strtok_r(NULL, "-", &arg2));

                if (country != NULL) {
                    sendToWorkerHavingCountry(country, query, socketFd);
                } else{
                    sendToWorkerSimple(query, socketFd);
                }
                free(date1);
                free(date2);
            }

        }else if (strcmp(command, "/topk-AgeRanges") == 0) {
            strtok_r(NULL, " ", &saveptr);
            char *country = strtok_r(NULL, " ", &saveptr);
            strtok_r(NULL, " ", &saveptr);
            char *arg3 = strtok_r(NULL, " ", &saveptr);
            char *arg4 = strtok_r(NULL, " ", &saveptr);

            if (arg3 != NULL && arg4 != NULL) {
                Date *date1 = malloc(sizeof(struct Date));
                Date *date2 = malloc(sizeof(struct Date));
                date1->day = atoi(strtok_r(arg3, "-", &saveptr));
                date1->month = atoi(strtok_r(NULL, "-", &saveptr));
                date1->year = atoi(strtok_r(NULL, "-", &saveptr));
                date2->day = atoi(strtok_r(arg4, "-",&saveptr));
                date2->month = atoi(strtok_r(NULL, "-", &saveptr));
                date2->year = atoi(strtok_r(NULL, "-", &saveptr));

                sendToWorkerHavingCountry(country, query, socketFd);

                free(date1);
                free(date2);

            }

        } else if (strcmp(command, "/searchPatientRecord") == 0) {
            strtok_r(NULL, "\n", &saveptr);
            sendToWorkerSimple(query, socketFd);

        } else if (strcmp(command, "/numPatientAdmissions") == 0 || strcmp(command, "/numPatientDischarges") == 0) {
            Date *date1;
            Date *date2;
            date1 = malloc(sizeof(struct Date));
            date2 = malloc(sizeof(struct Date));

            strtok_r(NULL, " ", &saveptr);   //virus
            char *arg2 = strtok_r(NULL, " ", &saveptr);   //date1
            char *arg3 = strtok_r(NULL, " ", &saveptr);   //date2
            char *country = strtok_r(NULL, " ", &saveptr);

            if (arg2 != NULL && arg3 != NULL) {
                date1->day = atoi(strtok_r(arg2, "-", &saveptr));
                date1->month = atoi(strtok_r(NULL, "-", &saveptr));
                date1->year = atoi(strtok_r(NULL, "-", &saveptr));
                date2->day = atoi(strtok_r(arg3, "-", &saveptr));
                date2->month = atoi(strtok_r(NULL, "-", &saveptr));
                date2->year = atoi(strtok_r(NULL, "-", &saveptr));

                if (country != NULL) {
                    sendToWorkerHavingCountry(country, query, socketFd);
                } else
                    sendToWorkerSimple(query, socketFd);

            }

            free(date1);
            free(date2);

        }
    }

}

void sendToWorkerSimple(char* query, int socketFd){
    struct sockaddr *serverptr;
    struct sockaddr_in worker;
    int newSocket;
    char * answer;
    socklen_t workerlen;

    int sum = 0;
    char* tmpAnwser = calloc(sizeof(char), MESSAGE_BUFFER);
    bool sendFlag = false;
    bool intFlag = false;

    for(int i = 0; i < whoServerManager->numOfWorkers; i++){
        /* Find worker address */
        if ((newSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0){
            perror("worker socket");
            exit(1);
        }
        worker.sin_family = AF_INET;
        worker.sin_addr.s_addr = whoServerManager->serverSocket->socketAddressServer.sin_addr.s_addr;
        worker.sin_port = htons(whoServerManager->workerItemArray[i].workerPort);         /* Server port */

        serverptr = (struct sockaddr *) &(worker);
        workerlen = sizeof(worker);
        if (connect(newSocket, serverptr, workerlen) < 0) {
            perror("Connect worker");
            exit(1);
        }
        fprintf(stderr, "Connecting to worker server port %d to send query\n",
                whoServerManager->workerItemArray[i].workerPort);

        write(newSocket, query, MESSAGE_BUFFER);
        answer = calloc(sizeof(char), MESSAGE_BUFFER);
        char* saveptr;
        char* command = strdup(query);
        command = strtok_r(command, " ", &saveptr);
        if(strcmp(command, "/diseaseFrequency") == 0) {         /*for number results*/
            read(newSocket, answer, MESSAGE_BUFFER);
            sum += atoi(answer);
            sendFlag = true;
            intFlag = true;
            free(answer);
        }else if(strcmp(command, "/numPatientAdmissions") == 0 || strcmp(command, "/numPatientDischarges") == 0){   /*for string concatenation*/
            read(newSocket, answer, MESSAGE_BUFFER);
            strcat(tmpAnwser, answer);
            printf("%s", tmpAnwser);
            sendFlag = true;
        }else{
            read(newSocket, answer, MESSAGE_BUFFER);
            write(socketFd, answer, MESSAGE_BUFFER);
            free(answer);
        }

    }
    if(sendFlag == true){
        if(intFlag == true) {
            answer = calloc(sizeof(char), MESSAGE_BUFFER);
            sprintf(answer, "%d", sum);
            write(socketFd, answer, MESSAGE_BUFFER);
            free(answer);
        } else{
            write(socketFd, tmpAnwser, MESSAGE_BUFFER);
        }
        free(tmpAnwser);
    }

}




void sendToWorkerHavingCountry(char* country, char* query, int socketFd){

    struct sockaddr *serverptr;
    struct sockaddr_in worker;
    int newSocket;
    char * answer;
    socklen_t workerlen;

    for(int i = 0; i < whoServerManager->numOfWorkers; i++) {

        pthread_mutex_lock(&(whoServerManager->mtxW));

        if (workerHasCountry(country, (whoServerManager->workerItemArray[i].list))) {
            pthread_mutex_unlock(&(whoServerManager->mtxW));
            /* Find server address */
            if ((newSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                perror("client socket");
                exit(1);
            }
            worker.sin_family = AF_INET;
            worker.sin_addr = whoServerManager->serverSocket->socketAddressClient.sin_addr;
            worker.sin_port = htons(whoServerManager->workerItemArray[i].workerPort);         /* Server port */

            serverptr = (struct sockaddr *) &(worker);
            workerlen = sizeof(worker);
            if (connect(newSocket, serverptr, workerlen) < 0) {
                perror("Connect worker");
                exit(1);
            }
            fprintf(stderr, "Connecting to worker server port %d to send query\n",
                    whoServerManager->workerItemArray[i].workerPort);

            write(newSocket, query, MESSAGE_BUFFER);
            answer = calloc(sizeof(char), MESSAGE_BUFFER);
            fprintf(stderr, "Waiting for results\n");
            //do {
            read(newSocket, answer, MESSAGE_BUFFER);
            //fprintf(stderr, "--%s\n", answer);
            write(socketFd, answer, MESSAGE_BUFFER);
            //} while (strcmp(answer, "$END$") != 0);
            break;
        } else {
            pthread_mutex_unlock(&(whoServerManager->mtxW));
        }
    }
}

bool workerHasCountry(char *country, List *workerCountryList){
    Node* node = workerCountryList->head;
    while (node != NULL){
        if (compareListItemCountry(node->item, country)){
            return true;
        }
        node = node->next;
    }
    return false;
}

bool compareListItemCountry(CountryListItem* countryItem, char* key){
    //fprintf(stderr,"#############%s %s\n", countryItem->country, key);
    if (strcmp(countryItem->country, key) == 0){
        return true;
    }
    return false;
}

void printCountryList(List *countryList){
    Node* node = countryList->head;
    CountryListItem *countryListItem;
    while (node != NULL){
        countryListItem = node->item;
        if(countryListItem->country == NULL)
            fprintf(stderr, "Shit just went sideways in the most colossal way.\n");
        else
            fprintf(stderr, "%s\n\n", countryListItem->country);
        node = node->next;
    }
}