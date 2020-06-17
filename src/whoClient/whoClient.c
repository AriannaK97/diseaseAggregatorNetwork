//
// Created by AriannaK97 on 11/6/20.
//

#include "whoClientIO.h"

int main (int argc, char** argv){

/*****************************************************************************
 *                       Handling command line arguments                     *
 *****************************************************************************/
    WhoClientInputArguments *arguments = getWhoClientArguments(argc, argv);

    whoClientManager = initializeWhoClientManager(arguments);

    freeWhoClientInputArgs(arguments);

}

