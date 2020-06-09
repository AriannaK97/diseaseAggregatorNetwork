//
// Created by AriannaK97 on 21/5/20.
//
#define  _GNU_SOURCE
#include <sys/wait.h>
#include <stdio.h>
#include <fcntl.h>
#include "../../header/diseaseAggregator.h"
#include "../../header/command_lib.h"
#include "../../header/communication.h"

bool sig_flag = false;
bool childJustDies = false;

AggregatorServerManager* readDirectoryFiles(AggregatorInputArguments* arguments){
    DIR* FD;
    DIR* SubFD;
    struct dirent* in_dir;
    char *dirPath = (char*)malloc(DIR_LEN*sizeof(char));
    char *subDirPath = (char*)malloc(DIR_LEN*sizeof(char));
    //AggregatorServerManager* aggregatorManager;
    int distributionPointer = 0;
    DirListItem* newItem = NULL;
    Node* listNode = NULL;

    /* Scanning the in directory */
    if (NULL == (FD = opendir (arguments->input_dir))){
        fprintf(stderr, "Error : Failed to open input directory - %s\n", strerror(errno));
        exit(1);
    }
    strcpy(dirPath, arguments->input_dir);
    strcat(dirPath, "/");
    int numOfSubDirectories = 0;
    while((in_dir = readdir(FD))){
        if (!strcmp (in_dir->d_name, "."))
            continue;
        if (!strcmp (in_dir->d_name, ".."))
            continue;
        numOfSubDirectories+=1;
    }
    rewinddir(FD);

    aggregatorServerManager = (AggregatorServerManager*)malloc(sizeof(AggregatorServerManager));
    /*array of lists - each list holds the directories assigned to each worker*/
    if(arguments->numWorkers > numOfSubDirectories){
        arguments->numWorkers = numOfSubDirectories;    /*if the workers are more than the subdirs*/
    }
    aggregatorServerManager->success = 0;
    aggregatorServerManager->fail = 0;
    aggregatorServerManager->line = NULL;
    aggregatorServerManager->directoryDistributor = (List**)calloc(sizeof(List*),arguments->numWorkers);
    for (int i = 0; i < arguments->numWorkers; ++i) {
        aggregatorServerManager->directoryDistributor[i] = NULL;
    }

    while ((in_dir = readdir(FD))){
        if(distributionPointer >= arguments->numWorkers){
            distributionPointer = 0;
        }
        if (!strcmp (in_dir->d_name, "."))
            continue;
        if (!strcmp (in_dir->d_name, ".."))
            continue;
        /*create current deirectory's path*/
        strcpy(subDirPath, dirPath);
        strcat(subDirPath, in_dir->d_name);

        /* Open subdirestory*/
        if (NULL == (SubFD = opendir (subDirPath))){
            fprintf(stderr, "Error : Failed to open input directory %s - %s\n",subDirPath, strerror(errno));
            exit(1);
        }

        newItem = (struct DirListItem*)malloc(sizeof(struct DirListItem));
        newItem->dirPath = (char*)calloc(sizeof(char), strlen(subDirPath) + 1);
        newItem->dirName = (char*)calloc(sizeof(char), strlen(in_dir->d_name) + strlen(subDirPath) + 1);

        strcpy(newItem->dirName, in_dir->d_name);
        strcpy(newItem->dirPath, subDirPath);

        listNode = nodeInit(newItem);
        //printf("%s\n", in_dir->d_name);
        if(aggregatorServerManager->directoryDistributor[distributionPointer] == NULL){
            aggregatorServerManager->directoryDistributor[distributionPointer] = linkedListInit(listNode);
        } else {
            push(listNode, aggregatorServerManager->directoryDistributor[distributionPointer]);
        }

        distributionPointer++;

        strcpy(dirPath, arguments->input_dir);
        strcat(dirPath, "/");
        closedir(SubFD);
    }

    closedir(FD);
    free(dirPath);
    free(subDirPath);
    return aggregatorServerManager;
}


void printAggregatorManagerDirectoryDistributor(AggregatorServerManager* aggregatorManager, int numOfWorkers){
    Node* currentNode;
    DirListItem* item;
    int counter;
    for (int i = 0; i < numOfWorkers; ++i) {
        counter = 0;
        currentNode = (Node*)aggregatorManager->directoryDistributor[i]->head;
        fprintf(stdout, "---------------worker %d--------------\n", i);
        while (currentNode != NULL){
            item = (DirListItem*)currentNode->item;
            fprintf(stdout, "\tdirectory: %s\n", item->dirName);
            currentNode = currentNode->next;
            counter +=1;
        }
        fprintf(stdout, "worker%d serves: %d directories\n\n", i, counter);
    }
}

bool make_fifo_name_server_client(pid_t workerNum, char *name){
    sprintf(name, "workerFromServer%d", workerNum);
    return true;
}

bool make_fifo_name_client_server(pid_t workerNum, char *name){
    sprintf(name, "workerFromClient%d", workerNum);
    return true;
}

int countFilesInDirectory(DIR *FD){
    struct dirent* in_file;
    int counter = 0;
    while ((in_file = readdir(FD))){
        if (!strcmp(in_file->d_name, "."))
            continue;
        if (!strcmp(in_file->d_name, ".."))
            continue;

        counter += 1;
    }
    rewinddir(FD);
    return counter;
}

FileItem* createFileArray(DIR * FD, DirListItem* item, int arraySize, int bufferSize){

    struct dirent* in_file;
    char *subDirPath = calloc(sizeof(char), bufferSize + 1);
    FileItem* fileArray = (struct FileItem*)calloc(sizeof(struct FileItem),arraySize);
    int filePointer = 0;
    char* temp =  calloc(sizeof(char), bufferSize + 1);

    while ((in_file = readdir(FD))) {

        if (!strcmp(in_file->d_name, "."))
            continue;
        if (!strcmp(in_file->d_name, ".."))
            continue;

        fileArray[filePointer].dateFile =  malloc(sizeof(struct Date));

        fileArray[filePointer].filePath = calloc(sizeof(char), bufferSize + 1);
        fileArray[filePointer].fileName = calloc(sizeof(char), bufferSize + 1);
        fileArray[filePointer].numOfDiseases = 0;

        strcpy(subDirPath, item->dirPath);
        strcat(subDirPath, "/");
        strcat(subDirPath, in_file->d_name);
        strcpy(temp, in_file->d_name);

        fileArray[filePointer].dateFile->day = atoi(strtok(temp, "-"));
        fileArray[filePointer].dateFile->month = atoi(strtok(NULL, "-"));
        fileArray[filePointer].dateFile->year = atoi(strtok(NULL, "-"));

        strcpy(fileArray[filePointer].filePath, subDirPath);
        strcpy(fileArray[filePointer].fileName, in_file->d_name);

        //fprintf(stdout, "%s \n", fileItemsArray[filePointer].filePath);

        filePointer++;
    }
    qsort(fileArray, arraySize, sizeof(FileItem), compare);
    rewinddir(FD);
    free(temp);
    return fileArray;
}


bool sendStatistics(CmdManager* cmdManager) {

    char* messageSize;
    char* message;

    /*write the number of directories that will send stats to follow to fifo*/
    message = calloc(sizeof(char), cmdManager->bufferSize + 1);
    sprintf(message, "%d", cmdManager->numOfDirectories);
    writeInFifoPipe(cmdManager->fd_client_w, message, (cmdManager->bufferSize) + 1);
    free(message);
    /*send statistics*/
    for (int i = 0; i < cmdManager->numOfDirectories; i++) {
        /*write the country*/
        writeInFifoPipe(cmdManager->fd_client_w, cmdManager->fileExplorer[i]->country,(cmdManager->bufferSize) + 1);

        /*write number of files for the country*/
        messageSize = calloc(sizeof(char), cmdManager->bufferSize + 1);
        sprintf(messageSize, "%d", cmdManager->fileExplorer[i]->fileArraySize);
        writeInFifoPipe(cmdManager->fd_client_w, messageSize, (cmdManager->bufferSize)  + 1);
        free(messageSize);
        for (int j = 0; j < cmdManager->fileExplorer[i]->fileArraySize; j++) {
            /*write the file name*/

            writeInFifoPipe(cmdManager->fd_client_w, cmdManager->fileExplorer[i]->fileItemsArray[j].fileName,
                                (cmdManager->bufferSize) + 1);

            /*write number of diseases for the country*/
            messageSize = calloc(sizeof(char), cmdManager->bufferSize + 1);
            sprintf(messageSize, "%d", cmdManager->fileExplorer[i]->fileItemsArray[j].numOfDiseases);
            writeInFifoPipe(cmdManager->fd_client_w, messageSize, (cmdManager->bufferSize)  + 1);
            free(messageSize);
            for (int k = 0; k < cmdManager->fileExplorer[i]->fileItemsArray[j].numOfDiseases; k++) {
                /*write disease*/

                writeInFifoPipe(cmdManager->fd_client_w,
                                    cmdManager->fileExplorer[i]->fileItemsArray[j].fileDiseaseStats[k]->disease,(cmdManager->bufferSize) + 1);
                /*write stats for age ranges*/
                for (int l = 0; l < 4; l++) {
                    message = calloc(sizeof(char), (cmdManager->bufferSize) + 1);
                    if(l == 0){
                        sprintf(message, "Age range 0-20 years: %d cases", cmdManager->fileExplorer[i]->fileItemsArray[j].fileDiseaseStats[k]->AgeRangeCasesArray[l]);
                    }else if(l == 1){
                        sprintf(message, "Age range 21-40 years: %d cases", cmdManager->fileExplorer[i]->fileItemsArray[j].fileDiseaseStats[k]->AgeRangeCasesArray[l]);
                    }else if(l == 2){
                        sprintf(message, "Age range 41-60 years: %d cases", cmdManager->fileExplorer[i]->fileItemsArray[j].fileDiseaseStats[k]->AgeRangeCasesArray[l]);
                    }else if(l == 3){
                        sprintf(message, "Age range 60+ years: %d cases", cmdManager->fileExplorer[i]->fileItemsArray[j].fileDiseaseStats[k]->AgeRangeCasesArray[l]);
                    }

                    writeInFifoPipe(cmdManager->fd_client_w, message,(cmdManager->bufferSize) + 1);

                    free(message);
                }
                /*end of stat batch*/

                writeInFifoPipe(cmdManager->fd_client_w, "next",(cmdManager->bufferSize) + 1);
            }
        }
    }

    /*send end of transmission message*/
    writeInFifoPipe(cmdManager->fd_client_w, "StatsDone", cmdManager->bufferSize);
    return true;
}

void deallockWorkerInfo(WorkerInfo* workerInfo){
    free(workerInfo->workerFileName);
    free(workerInfo->serverFileName);
    free(workerInfo);
}

bool receiveStats(AggregatorServerManager* aggregatorServerManager, int workerId){
    char *country, *fileName, *disease, *message, *messageSize;
    int numOfDirs, numOfFiles, numOfDiseases;

    /*read per country*/
    message = calloc(sizeof(char), aggregatorServerManager->bufferSize + 1);
    readFromFifoPipe(aggregatorServerManager->workersArray[workerId].fd_client_r, message, (aggregatorServerManager->bufferSize)+1);
    numOfDirs = atoi(message);
    free(message);

    for (int i = 0; i < numOfDirs; i++) {

        /*read actual message from fifo*/
        country = calloc(sizeof(char), (aggregatorServerManager->bufferSize)+1);
        readFromFifoPipe(aggregatorServerManager->workersArray[workerId].fd_client_r, country,(aggregatorServerManager->bufferSize)+1);

        /*read per file*/
        messageSize = calloc(sizeof(char), (aggregatorServerManager->bufferSize) + 1);
        readFromFifoPipe(aggregatorServerManager->workersArray[workerId].fd_client_r, messageSize, (aggregatorServerManager->bufferSize)+1);
        numOfFiles = atoi(messageSize);
        free(messageSize);
        for (int j = 0; j < numOfFiles; j++) {

            /*read actual message from fifo*/
            fileName = calloc(sizeof(char), (aggregatorServerManager->bufferSize)+1);
            readFromFifoPipe(aggregatorServerManager->workersArray[workerId].fd_client_r, fileName,(aggregatorServerManager->bufferSize)+1);
            fprintf(stdout, "\n%s\n%s\n",fileName, country);

            /*read per disease*/
            messageSize = calloc(sizeof(char), aggregatorServerManager->bufferSize + 1);
            readFromFifoPipe(aggregatorServerManager->workersArray[workerId].fd_client_r, messageSize, (aggregatorServerManager->bufferSize)+1);
            numOfDiseases = atoi(messageSize);
            free(messageSize);
            for (int k = 0; k < numOfDiseases; k++) {
                /*read actual message from fifo*/
                disease = calloc(sizeof(char), (aggregatorServerManager->bufferSize)+1);
                readFromFifoPipe(aggregatorServerManager->workersArray[workerId].fd_client_r, disease,(aggregatorServerManager->bufferSize)+1);
                fprintf(stdout, "%s\n", disease);

                for (int l = 0; l < 4; l++) {
                    /*read actual message from fifo*/
                    message = calloc(sizeof(char), aggregatorServerManager->bufferSize+1);
                    readFromFifoPipe(aggregatorServerManager->workersArray[workerId].fd_client_r, message,(aggregatorServerManager->bufferSize)+1);
                    fprintf(stdout, "%s\n", message);
                    free(message);
                }

                /*read actual message from fifo*/
                message = calloc(sizeof(char), (aggregatorServerManager->bufferSize)+1);
                readFromFifoPipe(aggregatorServerManager->workersArray[workerId].fd_client_r, message,(aggregatorServerManager->bufferSize)+1);
                free(message);
                free(disease);
            }
            free(fileName);
        }
        free(country);
    }
    message = calloc(sizeof(char), (aggregatorServerManager->bufferSize)+1);
    readFromFifoPipe(aggregatorServerManager->workersArray[workerId].fd_client_r, message,(aggregatorServerManager->bufferSize)+1);
    free(message);
    return true;
}


AggregatorInputArguments* getAggregatorInputArgs(int argc, char** argv){

    AggregatorInputArguments* arguments =  malloc(sizeof(struct AggregatorInputArguments));
    int numOfArgs = 0;
    if(argc != 7){
        fprintf(stderr, "Invalid number of arguments\nExit...\n");
        exit(1);
    }
    for (int i = 1; i < argc; i += 2) {
        if (strcmp(argv[i], "-i") == 0) {
            arguments->input_dir = malloc(sizeof(char)*DIR_LEN);
            strcpy(arguments->input_dir, argv[i + 1]);
            numOfArgs += 2;
        } else if (strcmp(argv[i], "-w") == 0) {
            arguments->numWorkers = atoi(argv[i + 1]);
            numOfArgs += 2;
        } else if (strcmp(argv[i], "-b") == 0) {
            arguments->bufferSize = atoi(argv[i + 1]);
            if(arguments->bufferSize < 120){
                arguments->bufferSize = 120;
            }
            numOfArgs += 2;
        } else {
            fprintf(stderr, "Unknown option %s\n", argv[i]);
            exit(1);
        }
    }
    if (arguments->input_dir == NULL) {
        fprintf(stdout, "Default file patientRecordsFile loaded...\n");
    }

    return arguments;
}


void exitAggregator(AggregatorServerManager* pAggregatorServerManager){
    char* answer;
    char* message = calloc(sizeof(char), pAggregatorServerManager->bufferSize+1);
    strcpy(message, "/exit");
    stayDead = true;

    for (int i = 0; i < pAggregatorServerManager->numOfWorkers; i++){
        writeInFifoPipe(pAggregatorServerManager->workersArray[i].fd_client_w, message, pAggregatorServerManager->bufferSize + 1 );
        answer = calloc(sizeof(char), pAggregatorServerManager->bufferSize + 1);
        readFromFifoPipe(pAggregatorServerManager->workersArray[i].fd_client_r, answer, pAggregatorServerManager->bufferSize + 1);
        if(strcmp(answer, "kill me father, for I have sinned") == 0) {
            free(answer);
            continue;
        }
    }

    for (int j = 0; j < pAggregatorServerManager->numOfWorkers; ++j) {
        kill(pAggregatorServerManager->workersArray[j].workerPid,SIGKILL);
    }
    freeAggregatorManager(pAggregatorServerManager);
    free(message);

    exit(0);
}


void DiseaseAggregatorServerManager(AggregatorServerManager* pAggregatorServerManager){

    char* command = NULL;
    char* simpleCommand = NULL;
    char* arguments = NULL;
    char* message = NULL;
    char* answer = NULL;
    size_t length = 0;

    fprintf(stdout,"~$:");
    while (1){

        getline(&(pAggregatorServerManager->line), &length, stdin);
        if(sig_flag){
            strcpy(pAggregatorServerManager->line,"\n");
            clearerr(stdin);
            sig_flag = false;
        }

        simpleCommand = strtok(pAggregatorServerManager->line, "\n");
        if(simpleCommand == NULL){
            pAggregatorServerManager->fail +=1;
            continue;
        }else if(strcmp(simpleCommand, "/help") == 0){
            helpDesc();
        } else if(strcmp(simpleCommand, "/exit") == 0){
            free(simpleCommand);
            pAggregatorServerManager->success +=1;
            exitAggregator(pAggregatorServerManager);
            break;
        }else if(strcmp(simpleCommand, "/listCountries") == 0){
            listCountries(pAggregatorServerManager);
            pAggregatorServerManager->success +=1;
        }else {

            command = strtok(simpleCommand, " ");
            arguments = strtok(NULL, "\n");
            if(strcmp(command, "/diseaseFrequency") == 0 || strcmp(command, "/topk-AgeRanges") == 0 ||
                        strcmp(command, "/searchPatientRecord") == 0 || strcmp(command, "/numPatientAdmissions") == 0
                        || strcmp(command, "/numPatientDischarges") == 0){

                message = calloc(sizeof(char), pAggregatorServerManager->bufferSize + 1);
                sprintf(message, "%s %s", command, arguments);

                if(strcmp(command, "/diseaseFrequency") == 0 ){
                    int total = 0;
                    for (int i = 0; i < pAggregatorServerManager->numOfWorkers; i++) {
                        writeInFifoPipe(pAggregatorServerManager->workersArray[i].fd_client_w, message, pAggregatorServerManager->bufferSize + 1 );
                        answer = calloc(sizeof(char), aggregatorServerManager->bufferSize + 1);
                        readFromFifoPipe(pAggregatorServerManager->workersArray[i].fd_client_r, answer, pAggregatorServerManager->bufferSize + 1);
                        total += atoi(answer);
                    }
                    if(strcmp(answer, "null")!=0) {
                        pAggregatorServerManager->success +=1;
                        fprintf(stdout, "%d\n", total);
                    } else{
                        pAggregatorServerManager->fail +=1;
                    }
                }else{
                    for (int i = 0; i < pAggregatorServerManager->numOfWorkers; i++) {
                        writeInFifoPipe(pAggregatorServerManager->workersArray[i].fd_client_w, message, pAggregatorServerManager->bufferSize + 1 );
                        answer = calloc(sizeof(char), pAggregatorServerManager->bufferSize + 1);
                        readFromFifoPipe(pAggregatorServerManager->workersArray[i].fd_client_r, answer, pAggregatorServerManager->bufferSize + 1);
                        if(strcmp(answer, "null")!=0){
                            pAggregatorServerManager->success +=1;
                            fprintf(stdout, "%s", answer);
                        } else{
                            pAggregatorServerManager->fail +=1;
                        }
                        free(answer);
                    }
                }
                fprintf(stdout, "~$:");
                free(message);
                //free(command);
            }else{
                fprintf(stdout,"The command you have entered does not exist.\n You can see the "
                               "available commands by hitting /help.\n~$:");
            }
        }
    }

}


void commandServer(CmdManager* manager) {
    char *command = NULL;
    char *simpleCommand = NULL;
    char *line = calloc(sizeof(char), (manager->bufferSize) + 1);
    int reader = -1;

    do{

        reader = read(manager->fd_client_r, line, manager->bufferSize + 1);

        if(sig_flag && reader < 0){
            reader = read(manager->fd_client_r, line, manager->bufferSize + 1);
            sig_flag = false;
        }

        if (reader < 0) {
            break;
        }
        simpleCommand = strtok(line, "\n");
        if (simpleCommand == NULL) {
            manager->workerLog->fails+=1;
            continue;
        } else if (strcmp(simpleCommand, "/help") == 0) {
            manager->workerLog->successes+=1;
            helpDesc();
        } else if (strcmp(simpleCommand, "/exit") == 0) {
            manager->workerLog->successes+=1;
            free(line);
            exitMonitor(manager);
        } else {

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
                    manager->workerLog->successes += 1;

                    if (country != NULL) {
                        diseaseFrequency(manager, virusName, date1, date2, country);
                    } else
                        diseaseFrequency(manager, virusName, date1, date2, NULL);

                    free(date1);
                    free(date2);
                }else{
                    manager->workerLog->fails+=1;
                }

            } else if (strcmp(command, "/topk-AgeRanges") == 0) {

                int k = atoi(strtok(NULL, " "));
                char *country = strtok(NULL, " ");
                char *disease = strtok(NULL, " ");
                char *arg3 = strtok(NULL, " ");
                char *arg4 = strtok(NULL, " ");

                if (arg3 != NULL && arg4 != NULL) {
                    Date *date1 = malloc(sizeof(struct Date));
                    Date *date2 = malloc(sizeof(struct Date));
                    date1->day = atoi(strtok(arg3, "-"));
                    date1->month = atoi(strtok(NULL, "-"));
                    date1->year = atoi(strtok(NULL, "-"));
                    date2->day = atoi(strtok(arg4, "-"));
                    date2->month = atoi(strtok(NULL, "-"));
                    date2->year = atoi(strtok(NULL, "-"));
                    manager->workerLog->successes+=1;
                    topk_AgeRanges(manager, k, country, disease, date1, date2);
                    free(date1);
                    free(date2);

                } else if (arg3 == NULL || arg4 == NULL) {
                    manager->workerLog->fails+=1;
                }

            } else if (strcmp(command, "/searchPatientRecord") == 0) {

                char *recordID = strtok(NULL, "\n");
                manager->workerLog->successes+=1;
                searchPatientRecord(manager, recordID);

            } else if (strcmp(command, "/numPatientAdmissions") == 0) {

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
                    manager->workerLog->successes+=1;

                    if (country != NULL) {
                        numPatientAdmissions(manager, virusName, date1, date2, country);
                    } else
                        numPatientAdmissions(manager, virusName, date1, date2, NULL);

                    free(date1);
                    free(date2);
                }else{
                    manager->workerLog->fails+=1;
                }

            } else if (strcmp(command, "/numPatientDischarges") == 0) {
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
                    manager->workerLog->successes+=1;
                    if (country != NULL) {
                        numPatientDischarges(manager, virusName, date1, date2, country);
                    } else
                        numPatientDischarges(manager, virusName, date1, date2, NULL);

                    free(date1);
                    free(date2);
                }else{
                    manager->workerLog->fails+=1;
                }
            } else{
                manager->workerLog->fails+=1;
            }
        }
    }while(reader > 0 );
}





void freeAggregatorManager(AggregatorServerManager *aggregatorManager){

    for (int i = 0; i < aggregatorManager->numOfWorkers; ++i) {
        close(aggregatorManager->workersArray[i].fd_client_w);
        close(aggregatorManager->workersArray[i].fd_client_r);
        remove(aggregatorManager->workersArray[i].serverFileName);
        remove(aggregatorManager->workersArray[i].workerFileName);
        dirListMemoryDeallock(aggregatorManager->directoryDistributor[i]);
        free(aggregatorManager->workersArray[i].workerFileName);
        free(aggregatorManager->workersArray[i].serverFileName);
    }
    free(aggregatorManager->directoryDistributor);
    free(aggregatorManager->workersArray);
    free(aggregatorManager->input_dir);
    //free(aggregatorServerManager->line);
    free(aggregatorManager);
}

void freeAggregatorInputArguments(AggregatorInputArguments *aggregatorInputArguments){
    free(aggregatorInputArguments->input_dir);
    free(aggregatorInputArguments);
}

void deallockFileItem(FileItem* fileItem){
    free(fileItem->fileName);
    free(fileItem->filePath);
    free(fileItem->dateFile);
    for (int i = 0; i < fileItem->numOfDiseases; ++i) {
        deallockFileDiseaseStats(fileItem->fileDiseaseStats[i]);
    }
    free(fileItem);
}

void nodeDirListItemDeallock(DirListItem* dirListItem){
    free(dirListItem->dirPath);
    free(dirListItem->dirName);
    free(dirListItem);
}


/*****************************************************************************
 *                         Signal Handling for Server                        *
 *****************************************************************************/

void respawnWorker(int sig){
    sig_flag = true;
    pid_t pid;
    int status;
    char *message;
    Node* currentNode;
    DirListItem* item;
    pid_t deadWorkerPid;
    deadWorkerPid = waitpid(-1, &status, WNOHANG);

    if(!stayDead){
        fprintf(stdout,"I might be Kronos for all you know...\n");
        for (int i = 0; i < aggregatorServerManager->numOfWorkers; i++) {
            if (aggregatorServerManager->workersArray[i].workerPid == deadWorkerPid) {
                fprintf(stdout,"Respawning...\n");

                if ((pid = fork()) == -1) {
                    perror("fork error");
                    exit(1);
                } else if (pid == 0) {
                    char* bufferSize_str = (char*)malloc(sizeof(char)*(aggregatorServerManager->bufferSize)+1);
                    char* workerId_str = (char*)malloc(sizeof(char)*(aggregatorServerManager->bufferSize)+1);
                    sprintf(bufferSize_str, "%zu", aggregatorServerManager->bufferSize);
                    sprintf(workerId_str, "%d", i);
                    execlp("./diseaseMonitor_client", "./diseaseMonitor_client", bufferSize_str, "5", "5" , "256",
                           aggregatorServerManager->input_dir, workerId_str, (char*)NULL);
                    fprintf(stdout,"Return not expected. Must be an execv error.n\n");
                    free(bufferSize_str);
                }

                aggregatorServerManager->workersArray[i].workerPid = pid;

                make_fifo_name_server_client(pid, aggregatorServerManager->workersArray[i].serverFileName);
                createNewFifoPipe(aggregatorServerManager->workersArray[i].serverFileName);
                aggregatorServerManager->workersArray[i].fd_client_w = openFifoToWrite(
                        aggregatorServerManager->workersArray[i].serverFileName);

                /*send the length of the data the client has to read*/
                message = calloc(sizeof(char), aggregatorServerManager->bufferSize + 1);
                sprintf(message, "%d", aggregatorServerManager->directoryDistributor[i]->itemCount);
                writeInFifoPipe(aggregatorServerManager->workersArray[i].fd_client_w, message,
                                (aggregatorServerManager->bufferSize) + 1);
                free(message);

                currentNode = (Node *) aggregatorServerManager->directoryDistributor[i]->head;
                while (currentNode != NULL) {
                    item = currentNode->item;
                    /*write the size of the name of the directory to follow to fifo*/
                    /*write the directory name to fifo*/
                    message = calloc(sizeof(char), (aggregatorServerManager->bufferSize) + 1);
                    strcpy(message, item->dirName);
                    writeInFifoPipe(aggregatorServerManager->workersArray[i].fd_client_w, message,
                                    (size_t) (aggregatorServerManager->bufferSize) + 1);
                    free(message);

                    currentNode = currentNode->next;
                }


                /*start receiving*/
                make_fifo_name_client_server(pid, aggregatorServerManager->workersArray[i].workerFileName);
                createNewFifoPipe(aggregatorServerManager->workersArray[i].workerFileName);
                aggregatorServerManager->workersArray[i].fd_client_r = openFifoToRead(
                        aggregatorServerManager->workersArray[i].workerFileName);

                /*read actual message from fifo*/
                message = calloc(sizeof(char), (aggregatorServerManager->bufferSize) + 1);
                readFromFifoPipe(aggregatorServerManager->workersArray[i].fd_client_r, message,
                                 (aggregatorServerManager->bufferSize) + 1);

                fprintf(stdout, "%s\n", message);

                if (!receiveStats(aggregatorServerManager, i)) {
                    fprintf(stderr, "Could not receive statistics\n");
                }

                free(message);
                break;
            }
        }
    }
}


void aggregatorLogFile(int sig){
    stayDead = true;
    childJustDies = true;
    FILE * fptr;
    Node* currentNode;
    DirListItem* item;
    char* fileName = calloc(sizeof(char), 15);
    pid_t aggregatorPid = getpid();

    fprintf(stdout, "Family killing spree...\n");
    for (int j = 0; j < aggregatorServerManager->numOfWorkers; ++j) {
        kill(aggregatorServerManager->workersArray[j].workerPid, SIGKILL);
    }

    sprintf(fileName, "log_file.%d", aggregatorPid);
    fptr = fopen(fileName, "w");
    if(fptr < 0){
        /* File not created hence exit */
        perror("could not create log file for worker- ");
        exit(3);
    }
    for(int i = 0; i < aggregatorServerManager->numOfWorkers; i++) {
        currentNode = (Node *) aggregatorServerManager->directoryDistributor[i]->head;
        while (currentNode != NULL) {
            item = (DirListItem *) currentNode->item;
            fprintf(fptr, "%s\n", item->dirName);
            currentNode = currentNode->next;
        }

    }

    fprintf(fptr, "TOTAL %d\nSUCCESS %d\nFAIL %d\n", aggregatorServerManager->success + aggregatorServerManager->fail,
            aggregatorServerManager->success, aggregatorServerManager->fail);

    freeAggregatorManager(aggregatorServerManager);

    fclose(fptr);
    free(fileName);
    fprintf(stdout, "DEAD\n");
    exit(0);
}


void checkForNewFilesInSubDirs_handler(int sig){
    sig_flag = true;
    Node* node;
    FILE* entry_file;
    DIR* FD;
    DirListItem* item;
    int dirNum = 0;
    struct dirent* in_file;
    int previousNumOfFiles = 0;
    int fileArrayPosition = 0;
    bool fileExists = true;
    int numOfNewFiles = 0;
    char* messageSize;
    char* message;
    char* temp =  calloc(sizeof(char), cmdManager->bufferSize + 1);
    char *subDirPath = calloc(sizeof(char), cmdManager->bufferSize + 1);

    node = cmdManager->directoryList->head;
    while (node != NULL){
        item = (DirListItem*)node->item;
        node = node->next;
    }

    node = cmdManager->directoryList->head;
    while (node != NULL) {
        item = (DirListItem*)node->item;

        /* Scanning the in directory */
        if (NULL == (FD = opendir(item->dirPath))) {
            fprintf(stderr, "Error : Failed to open input directory - %s\n", strerror(errno));
            exit(1);
        }

        numOfNewFiles = countFilesInDirectory(FD);
        if(numOfNewFiles > cmdManager->fileExplorer[dirNum]->fileArraySize) {
            previousNumOfFiles = cmdManager->fileExplorer[dirNum]->fileArraySize;
            fileArrayPosition = cmdManager->fileExplorer[dirNum]->fileArraySize;
            cmdManager->fileExplorer[dirNum]->fileArraySize = numOfNewFiles;

            /*reallocate memory*/
            cmdManager->fileExplorer[dirNum]->fileItemsArray = (FileItem *) realloc(
                    cmdManager->fileExplorer[dirNum]->fileItemsArray,
                    sizeof(FileItem) * (cmdManager->fileExplorer[dirNum]->fileArraySize));



            while ((in_file = readdir(FD))) {
                if (!strcmp(in_file->d_name, "."))
                    continue;
                if (!strcmp(in_file->d_name, ".."))
                    continue;

                fileExists = false;
                for (int i = 0; i < previousNumOfFiles; i++) {
                    if(strcmp(cmdManager->fileExplorer[dirNum]->fileItemsArray[i].fileName, in_file->d_name) == 0){
                        fileExists = true;
                    }
                }

                if(!fileExists){

                    cmdManager->fileExplorer[dirNum]->fileItemsArray[fileArrayPosition].dateFile =  calloc(sizeof(struct Date), 1);
                    cmdManager->fileExplorer[dirNum]->fileItemsArray[fileArrayPosition].filePath = calloc(sizeof(char), cmdManager->bufferSize + 1);
                    cmdManager->fileExplorer[dirNum]->fileItemsArray[fileArrayPosition].fileName = calloc(sizeof(char), cmdManager->bufferSize + 1);
                    strcpy(cmdManager->fileExplorer[dirNum]->fileItemsArray[fileArrayPosition].fileName, in_file->d_name);
                    cmdManager->fileExplorer[dirNum]->fileItemsArray[fileArrayPosition].numOfDiseases = 0;

                    strcpy(subDirPath, item->dirPath);
                    strcat(subDirPath, "/");
                    strcat(subDirPath, in_file->d_name);
                    strcpy(temp, in_file->d_name);

                    cmdManager->fileExplorer[dirNum]->fileItemsArray[fileArrayPosition].dateFile->day = atoi(strtok(temp, "-"));
                    cmdManager->fileExplorer[dirNum]->fileItemsArray[fileArrayPosition].dateFile->month = atoi(strtok(NULL, "-"));
                    cmdManager->fileExplorer[dirNum]->fileItemsArray[fileArrayPosition].dateFile->year = atoi(strtok(NULL, "-"));

                    strcpy(cmdManager->fileExplorer[dirNum]->fileItemsArray[fileArrayPosition].filePath, subDirPath);

                    strcpy(cmdManager->fileExplorer[dirNum]->fileItemsArray[fileArrayPosition].fileName, in_file->d_name);

                    entry_file = fopen(cmdManager->fileExplorer[dirNum]->fileItemsArray[fileArrayPosition].filePath,"r");
                    if (entry_file == NULL) {
                        fprintf(stderr, "Error : Failed to open entry file %s %s - %s\n",
                                cmdManager->fileExplorer[dirNum]->fileItemsArray[fileArrayPosition].filePath,
                                cmdManager->fileExplorer[dirNum]->country, strerror(errno));
                        exit(1);
                    }

                    cmdManager = read_input_file(entry_file, getMaxFromFile(entry_file, LINE_LENGTH), cmdManager,
                                                 cmdManager->fileExplorer[dirNum], fileArrayPosition, true);

                    fclose(entry_file);
                    fileArrayPosition += 1;

                    /**send statistics*/
                    /*write the country*/
                    writeInFifoPipe(cmdManager->fd_client_w, cmdManager->fileExplorer[dirNum]->country,(cmdManager->bufferSize) + 1);

                    /*write the file name*/
                    writeInFifoPipe(cmdManager->fd_client_w, cmdManager->fileExplorer[dirNum]->fileItemsArray[previousNumOfFiles].fileName,
                                    (cmdManager->bufferSize) + 1);

                    /*write number of diseases for the country*/
                    messageSize = calloc(sizeof(char), cmdManager->bufferSize + 1);
                    sprintf(messageSize, "%d", cmdManager->fileExplorer[dirNum]->fileItemsArray[previousNumOfFiles].numOfDiseases);
                    writeInFifoPipe(cmdManager->fd_client_w, messageSize, (cmdManager->bufferSize)  + 1);
                    free(messageSize);

                    for (int k = 0; k < cmdManager->fileExplorer[dirNum]->fileItemsArray[previousNumOfFiles].numOfDiseases; k++) {
                        /*write disease*/

                        writeInFifoPipe(cmdManager->fd_client_w,
                                        cmdManager->fileExplorer[dirNum]->fileItemsArray[previousNumOfFiles].fileDiseaseStats[k]->disease,(cmdManager->bufferSize) + 1);
                        /*write stats for age ranges*/
                        for (int l = 0; l < 4; l++) {
                            message = calloc(sizeof(char), (cmdManager->bufferSize) + 1);
                            if(l == 0){
                                sprintf(message, "Age range 0-20 years: %d cases", cmdManager->fileExplorer[dirNum]->fileItemsArray[previousNumOfFiles].fileDiseaseStats[k]->AgeRangeCasesArray[l]);
                            }else if(l == 1){
                                sprintf(message, "Age range 21-40 years: %d cases", cmdManager->fileExplorer[dirNum]->fileItemsArray[previousNumOfFiles].fileDiseaseStats[k]->AgeRangeCasesArray[l]);
                            }else if(l == 2){
                                sprintf(message, "Age range 41-60 years: %d cases", cmdManager->fileExplorer[dirNum]->fileItemsArray[previousNumOfFiles].fileDiseaseStats[k]->AgeRangeCasesArray[l]);
                            }else if(l == 3){
                                sprintf(message, "Age range 60+ years: %d cases", cmdManager->fileExplorer[dirNum]->fileItemsArray[previousNumOfFiles].fileDiseaseStats[k]->AgeRangeCasesArray[l]);
                            }

                            writeInFifoPipe(cmdManager->fd_client_w, message,(cmdManager->bufferSize) + 1);

                            free(message);
                        }
                        /*end of stat batch*/
                        writeInFifoPipe(cmdManager->fd_client_w, "next",(cmdManager->bufferSize) + 1);
                    }
                    /*send end of transmission message*/
                    writeInFifoPipe(cmdManager->fd_client_w, "StatsDone", cmdManager->bufferSize);
                }
            }
        }
        closedir(FD);
        node = node->next;
        dirNum+=1;
    }
}



void debug(int signo, siginfo_t *si, void *data){
    sig_flag = true;
    (void)signo;
    (void)data;
    printf("hi, I am stupid!\n");
}


void receiveStatsUpdate(int signo, siginfo_t *si, void *data){
    sig_flag = true;
    pid_t childThatSignaled;
    int workerId;

    (void)signo;
    (void)data;

    childThatSignaled = (unsigned long)si->si_pid;
    for (int i = 0; i < aggregatorServerManager->numOfWorkers; ++i) {
        if(aggregatorServerManager->workersArray[i].workerPid == childThatSignaled){
            workerId = i;
            break;
        }
    }

    char *country, *fileName, *disease, *message, *messageSize;
    int numOfDiseases;

    /*receives country*/
    country = calloc(sizeof(char), (aggregatorServerManager->bufferSize)+1);
    readFromFifoPipe(aggregatorServerManager->workersArray[workerId].fd_client_r, country,(aggregatorServerManager->bufferSize)+1);

    /*receives filename*/
    fileName = calloc(sizeof(char), (aggregatorServerManager->bufferSize)+1);
    readFromFifoPipe(aggregatorServerManager->workersArray[workerId].fd_client_r, fileName,(aggregatorServerManager->bufferSize)+1);
    fprintf(stdout, "\n%s\n%s\n",fileName, country);

    /*read number of diseases*/
    messageSize = calloc(sizeof(char), aggregatorServerManager->bufferSize + 1);
    readFromFifoPipe(aggregatorServerManager->workersArray[workerId].fd_client_r, messageSize, (aggregatorServerManager->bufferSize)+1);
    numOfDiseases = atoi(messageSize);
    free(messageSize);
    for (int k = 0; k < numOfDiseases; k++) {
        /*read disease from fifo*/
        disease = calloc(sizeof(char), (aggregatorServerManager->bufferSize)+1);
        readFromFifoPipe(aggregatorServerManager->workersArray[workerId].fd_client_r, disease,(aggregatorServerManager->bufferSize)+1);
        fprintf(stdout, "%s\n", disease);

        for (int l = 0; l < 4; l++) {
            /*read actual message from fifo*/
            message = calloc(sizeof(char), aggregatorServerManager->bufferSize+1);
            readFromFifoPipe(aggregatorServerManager->workersArray[workerId].fd_client_r, message,(aggregatorServerManager->bufferSize)+1);
            fprintf(stdout, "%s\n", message);
            free(message);
        }
        /*read actual message from fifo*/
        message = calloc(sizeof(char), (aggregatorServerManager->bufferSize)+1);
        readFromFifoPipe(aggregatorServerManager->workersArray[workerId].fd_client_r, message,(aggregatorServerManager->bufferSize)+1);
        free(message);
        free(disease);
    }
    free(fileName);
    free(country);

    message = calloc(sizeof(char), (aggregatorServerManager->bufferSize)+1);
    readFromFifoPipe(aggregatorServerManager->workersArray[workerId].fd_client_r, message,(aggregatorServerManager->bufferSize)+1);
    free(message);
    fprintf(stdout, "~$:");
}

void sigintHandler(int sig){
    if(!childJustDies){
        FILE * fptr;
        DirListItem* item;
        Node* node = cmdManager->directoryList->head;
        char* fileName = calloc(sizeof(char), 15);
        sprintf(fileName, "log_file.%d", cmdManager->workerInfo->workerPid);
        fptr = fopen(fileName, "w");
        if(fptr < 0){
            /* File not created hence exit */
            perror("could not create log file for worker- ");
            exit(3);
        }

        while (node != NULL) {
            item = (DirListItem*)node->item;
            fprintf(fptr, "%s\n",item->dirName);
            node = node->next;
        }

        fprintf(fptr, "TOTAL %d\nSUCCESS %d\nFAIL %d", cmdManager->workerLog->successes + cmdManager->workerLog->fails,
                cmdManager->workerLog->successes, cmdManager->workerLog->fails);
        fprintf(stdout, "Please help - My dad is killing me...\n DEAD\n");

        fclose(fptr);
        free(fileName);
        exitMonitor(cmdManager);
    }
}



