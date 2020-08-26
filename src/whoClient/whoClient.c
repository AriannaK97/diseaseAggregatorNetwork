//
// Created by AriannaK97 on 11/6/20.
//

#include <stdio.h>
#include <unistd.h>
#include "../../header/whoClientIO.h"

int main (int argc, char** argv) {

/*****************************************************************************
 *                       Handling command line arguments                     *
 *****************************************************************************/
    WhoClientInputArguments *arguments = getWhoClientArguments(argc, argv);

    whoClientManager = initializeWhoClientManager(arguments);

    freeWhoClientInputArgs(arguments);

    /*Create threadpool*/
    ThreadPool *threadPool = initializeThreadpool(whoClientManager->numThreads, NULL,
                                                  atoi(whoClientManager->servIP), whoClientManager->servPort);

    readQueryFile(threadPool);

    //usleep(100000);
    pthread_cond_broadcast(&(threadPool->mutexCond));

    for(int i=0; i < whoClientManager->numThreads; i++){
        pthread_join(threadPool->threads[i], NULL);
    }

}

