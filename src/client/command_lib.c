//
// Created by AriannaK97 on 12/3/20.
//
#define  _GNU_SOURCE
#include "../../header/command_lib.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../../header/structs.h"
#include "../../header/hashTable.h"
#include "../../header/diseaseAggregator.h"
#include "../../header/communication.h"

/**
 * Prints every country along with its Worker's processID. It is usefull in case we want to add new files
 * for a certain country, and the user needs to find out which is the worker'r processID to send the necessary
 * signal to the certain worker.
 * */
void listCountries(AggregatorServerManager* pAggregatorServerManager){
    Node* currentNode = NULL;
    DirListItem* item = NULL;
    for (int i = 0; i < pAggregatorServerManager->numOfWorkers; ++i) {
        currentNode = (Node*)pAggregatorServerManager->directoryDistributor[i]->head;
        while (currentNode != NULL){
            item = currentNode->item;
            fprintf(stdout, "%s %d\n", item->dirName, pAggregatorServerManager->workersArray[i].workerPid);
            currentNode = currentNode->next;
        }
    }
    fprintf(stdout, "\n~$:");
}

/**
 * If the argument [country] is not define, the application prints for the virusName
 * defined the number of the diseased monitored in the system during the specified
 * period between [date1, date2]. If [country] is defined, the application prints
 * the number of the diseased in this [country] for the specified period.
 * Cmd Args: virusName date1 date2 [country]
 * */
void diseaseFrequency(CmdManager* manager, char* virusName, Date* date1, Date* date2, char* country){

    HashElement iterator = hashITERATOR(manager->diseaseHashTable);
    char* message = calloc(sizeof(char), manager->bufferSize + 1);
    iterator.date1 = date1;
    iterator.date2 = date2;
    iterator.virus = virusName;
    if(country == NULL) {
        while (hashIterateValues(&iterator, COUNT_ALL_BETWEEN_DATES_WITH_VIRUS) != NULL);
    }else {
        iterator.country = country;
        while (hashIterateValues(&iterator, COUNT_ALL_BETWEEN_DATES_WITH_VIRUS_AND_COUNTRY) != NULL);
    }

    if(iterator.counter == 0){
        sprintf(message, "null");
        manager->workerLog->fails+=1;
    }else{
        sprintf(message, "%d\n", iterator.counter);
        manager->workerLog->successes+=1;
    }
    writeInFifoPipe(manager->fd_client_w, message, manager->bufferSize + 1);
    //fprintf(stdout, "\n~$:");
}


/**
 * For the specified country and disease the application prints the age categories with the top k
 * number of diseased cases of the given disease during the period date1, date2, if that is specified.
 * Cmd Args: k country disease date1, date2
 * */
void topk_AgeRanges(CmdManager* manager, int k, char* country, char* disease , Date* date1, Date* date2){
    HashElement iterator = hashITERATOR(manager->diseaseHashTable);
    char* message = calloc(sizeof(char), manager->bufferSize + 1);
    AgeRange* ageRangeCasesArray = calloc(sizeof(AgeRange), 4);
    float total = 0;
    iterator.country = country;
    iterator.virus = disease;
    iterator.date1 = date1;
    iterator.date2 = date2;
    while (hashIterateValues(&iterator, GET_HEAP_NODES_AGE_RANGE_DATES) != NULL);

    if(iterator.AgeRangeNodes == NULL || iterator.AgeRangeNodes->head == NULL){
        //fprintf(stdout, "There are no countries with cases of %s\n~$:", disease);
        sprintf(message, "null");
        manager->workerLog->fails+=1;
        writeInFifoPipe(manager->fd_client_w, message, manager->bufferSize + 1);
        //freeHeapTree(maxHeap);
    }else{
        Node* currentNode = iterator.AgeRangeNodes->head;
        AgeRangeStruct* item;

        for (int j = 0; j < 4; ++j) {
            ageRangeCasesArray[j].index = j;
            ageRangeCasesArray[j].cases = 0;
        }

        while(currentNode != NULL){
            item = currentNode->item;
            total += item->dataSum;
            if (item->data <= 20){
                ageRangeCasesArray[0].cases = item->dataSum;
            }else if(item->data <= 40){
                ageRangeCasesArray[1].cases = item->dataSum;
            }else if(item->data <= 60){
                ageRangeCasesArray[2].cases = item->dataSum;
            }else if(item->data <= 120){
                ageRangeCasesArray[3].cases = item->dataSum;
            }
            currentNode = currentNode->next;
        }
        qsort(ageRangeCasesArray, 4, sizeof(AgeRange), compareTopkAgeRanges);
        if(k > 4){
            k = 4;
        }

        int stop = k;
        for (int i = 3; i >= 0; i--){
            char *temp = calloc(sizeof(char), 12);
            if (ageRangeCasesArray[i].index==0){
                sprintf(temp, "0-20: %.f%%\n", ((float)ageRangeCasesArray[i].cases)/total*100);
            }else if(ageRangeCasesArray[i].index==1){
                sprintf(temp, "21-40: %.f%%\n", ((float)ageRangeCasesArray[i].cases)/total*100);
            }else if(ageRangeCasesArray[i].index==2){
                sprintf(temp, "41-60: %.f%%\n", ((float)ageRangeCasesArray[i].cases)/total*100);
            }else if(ageRangeCasesArray[i].index==3){
                sprintf(temp, "60+: %.f%%\n", ((float)ageRangeCasesArray[i].cases)/total*100);
            } else{
                sprintf(temp, "null");
            }
            strcat(message, temp);
            stop--;
            free(temp);
            if(stop==0){
                break;
            }
        }
        free(ageRangeCasesArray);
        writeInFifoPipe(manager->fd_client_w, message, manager->bufferSize + 1);
    }
}

int compareTopkAgeRanges (const void * a, const void * b){
    AgeRange *aa = (AgeRange*)a;
    AgeRange *bb = (AgeRange*)b;
    return ((aa->cases)-(bb->cases));
}


/**
 * Search the record in system with the given recordID
 * Cmd Args: recordID
 * */
void searchPatientRecord(CmdManager* manager, char* recordID){
    PatientCase* patient;
    char* message = calloc(sizeof(char), manager->bufferSize + 1);
    patient = getPatientFromList(manager->patientList, recordID);
    if(patient == NULL){
        sprintf(message,"null");
        manager->workerLog->fails+=1;
    }else {
        if(patient->exitDate->day == 0){
            sprintf(message,"%s %s %s %s %d %d-%d-%d --\n", patient->recordID, patient->name, patient->surname, patient->virus,
                    patient->age, patient->entryDate->day, patient->entryDate->month, patient->entryDate->year);
        } else{
            sprintf(message,"%s %s %s %s %d %d-%d-%d %d-%d-%d\n", patient->recordID, patient->name, patient->surname, patient->virus,
                    patient->age, patient->entryDate->day, patient->entryDate->month, patient->entryDate->year,
                    patient->exitDate->day, patient->exitDate->month, patient->exitDate->year);
        }

    }
    writeInFifoPipe(manager->fd_client_w, message, manager->bufferSize + 1);
}


/**
 * If the argument [country] is given, the application prints the number of the
 * patients hospitalised from the specified country with the specified disease during
 * the period between the given dates.
 * If the [country] is not specified the application prints for all the countries
 * in the system the records of all the patients that are hospitalised with
 * the specified disease, during the period between the given dates.
 * Cmd Args: disease date1 date2 [country]
 * */
void numPatientAdmissions(CmdManager* manager, char* disease, Date* date1, Date* date2, char* country){
    int countryExists = false;
    char* message = calloc(sizeof(char), manager->bufferSize + 1);
    char* patientsNumStr = calloc(sizeof(char), 10);
    int patientsNum = 0;
    if(country != NULL){
        HashElement iterator = hashITERATOR(manager->countryHashTable);
        unsigned int h = hash(strlen(country)) % manager->countryHashTable->capacity;
        Bucket* bucket = manager->countryHashTable->table[h];
        iterator.date1 = date1;
        iterator.date2 = date2;
        iterator.virus = disease;
        iterator.country = country;
        if(bucket != NULL){
            while (bucket != NULL){
                for(int i = 0; i < bucket->numOfEntries; i++){
                    if(strcmp(country, bucket->entry[i].data)==0){
                        patientsNum = countPatients_BetweenDates(bucket->entry[i].tree, COUNT_HOSPITALISED_BETWEEN_DATES_WITH_DISEASE_AND_COUNTRY, &iterator);
                        if(strlen(bucket->entry[i].data)!=0){
                            strcat(message, country);
                            sprintf(patientsNumStr, " %d\n", patientsNum);
                            strcat(message, patientsNumStr);
                        }
                        countryExists = 1;
                        break;
                    }
                }
                if(countryExists)
                    break;
                bucket = bucket->next;
            }
            if(!countryExists || patientsNum == 0){
                sprintf(message, "null");
                manager->workerLog->fails+=1;
            }

        }else{
            sprintf(message, "null");
            manager->workerLog->fails+=1;
        }
        writeInFifoPipe(manager->fd_client_w, message, manager->bufferSize + 1);
        free(message);
    }else{
        HashElement iterator = hashITERATOR(manager->countryHashTable);
        int totalCounted = 0;
        iterator.date1 = date1;
        iterator.date2 = date2;
        iterator.virus = disease;
        bool foundAnswer = false;
        for (unsigned int h = 0; h < manager->countryHashTable->capacity; h++ ){
            Bucket* bucket = manager->countryHashTable->table[h];
            if(bucket != NULL){
                while (bucket != NULL){
                    for(int i = 0; i < bucket->numOfEntries; i++){
                        patientsNum = countPatients_BetweenDates(bucket->entry[i].tree, COUNT_HOSPITALISED_BETWEEN_DATES_WITH_DISEASE, &iterator);
                        if(strlen(bucket->entry[i].data)!=0 && patientsNum != 0) {
                            strcat(message, bucket->entry[i].data);
                            sprintf(patientsNumStr, " %d\n", patientsNum);
                            strcat(message, patientsNumStr);
                            foundAnswer = true;
                        }
                        totalCounted += patientsNum;
                    }
                    bucket = bucket->next;
                }
            }
        }
        if (!foundAnswer){
            sprintf(message, "null");
            manager->workerLog->fails+=1;
        }
        writeInFifoPipe(manager->fd_client_w, message, manager->bufferSize + 1);
        free(message);
    }
}

void numPatientDischarges(CmdManager* manager, char* disease, Date* date1, Date* date2, char* country){
    char* message = calloc(sizeof(char), manager->bufferSize + 1);
    char* patientsNumStr = calloc(sizeof(char), 10);
    int countryExists = false;
    int patientsNum = 0;
    bool foundAnswer = false;
    if(country != NULL){
        HashElement iterator = hashITERATOR(manager->countryHashTable);
        unsigned int h = hash(strlen(country)) % manager->countryHashTable->capacity;
        Bucket* bucket = manager->countryHashTable->table[h];
        iterator.date1 = date1;
        iterator.date2 = date2;
        iterator.virus = disease;
        iterator.country = country;
        if(bucket != NULL){
            while (bucket != NULL){
                for(int i = 0; i < bucket->numOfEntries; i++){
                    if(strcmp(country, bucket->entry[i].data)==0){
                        patientsNum = countPatients_BetweenDates(bucket->entry[i].tree, COUNT_HOSPITALISED_BETWEEN_DATES_WITH_DISEASE_AND_COUNTRY_EXIT, &iterator);
                        if(strlen(bucket->entry[i].data)!=0){
                            strcat(message, country);
                            sprintf(patientsNumStr, " %d\n", patientsNum);
                            strcat(message, patientsNumStr);
                        }
                        countryExists = 1;
                        break;
                    }
                }
                if(countryExists)
                    break;
                bucket = bucket->next;
            }
            if(!countryExists || patientsNum == 0){
                sprintf(message, "null");
                manager->workerLog->fails+=1;
            }
        }else {
            sprintf(message, "null");
            manager->workerLog->fails+=1;
        }
        writeInFifoPipe(manager->fd_client_w, message, manager->bufferSize + 1);
        free(message);
    }else{
        HashElement iterator = hashITERATOR(manager->countryHashTable);
        int totalCounted = 0;
        iterator.date1 = date1;
        iterator.date2 = date2;
        iterator.virus = disease;
        for (unsigned int h = 0; h < manager->countryHashTable->capacity; h++ ){
            Bucket* bucket = manager->countryHashTable->table[h];
            if(bucket != NULL){
                while (bucket != NULL){
                    for(int i = 0; i < bucket->numOfEntries; i++){
                        patientsNum = countPatients_BetweenDates(bucket->entry[i].tree, COUNT_HOSPITALISED_BETWEEN_DATES_WITH_DISEASE_EXIT, &iterator);
                        if(strlen(bucket->entry[i].data)!=0 && patientsNum != 0) {
                            strcat(message, bucket->entry[i].data);
                            sprintf(patientsNumStr, " %d\n", patientsNum);
                            strcat(message, patientsNumStr);
                            foundAnswer = true;
                        }
                        totalCounted += patientsNum;
                    }
                    bucket = bucket->next;
                }
            }
        }
        if (!foundAnswer){
            sprintf(message, "null");
            manager->workerLog->fails+=1;
        }
        writeInFifoPipe(manager->fd_client_w, message, manager->bufferSize + 1);
        free(message);
    }
}

/**
 * Exit from the application - Memory dialloccation
 * */
void exitMonitor(CmdManager* manager){
    FILE * fptr;
    DirListItem* item;
    char* message = calloc(sizeof(char), manager->bufferSize+1);
    Node* node = manager->directoryList->head;
    char* fileName = calloc(sizeof(char), 15);
    sprintf(fileName, "log_file.%d", manager->workerInfo->workerPid);
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

    fprintf(fptr, "TOTAL %d\nSUCCESS %d\nFAIL %d", manager->workerLog->successes+manager->workerLog->fails,
            manager->workerLog->successes, manager->workerLog->fails);

    fclose(fptr);
    free(fileName);

    //fprintf(stdout, "Destroying disease HashTable...\n");
    freeHashTable(manager->diseaseHashTable);

    //fprintf(stdout, "Destroying country HashTable...\n");
    freeHashTable(manager->countryHashTable);

    patientListMemoryDeallock(manager->patientList);

    dirListMemoryDeallock(manager->directoryList);

    for (int i = 0; i < manager->numOfDirectories; ++i) {
        deallockFileExplorer(manager->fileExplorer[i]);
    }
    //deallockWorkerInfo(manager->workerInfo);

    free(manager->workerLog);

    //free(manager->input_dir);

    strcpy(message, "kill me father, for I have sinned");
    writeInFifoPipe(manager->fd_client_w, message, manager->bufferSize + 1);

    close(manager->fd_client_r);
    close(manager->fd_client_w);

    remove(manager->workerInfo->workerFileName);
    remove(manager->workerInfo->serverFileName);

    free(manager);
    sleep(1);
    //exit(0);
}


/**
 * Help desc for all the actions of the application and how to call them.
 * */
void helpDesc(){
    fprintf(stdout,"/listCountries\n\n"
                   "/diseaseFrequency virusName date1 date2 [country]\n\n"
                   "/topk-AgeRanges k country disease date1 date2\n\n"
                   "/searchPatientRecord recordID\n\n"
                   "/numPatientAdmissions disease date1 date2 [country]\n\n"
                   "/numPatientDischarges disease date1 date2 [country]\n\n"
                   "/exit\n\n ~$:");
}



