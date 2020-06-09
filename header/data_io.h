//
// Created by AriannaK97 on 9/3/20.
//

#ifndef DISEASEMONITOR_DATA_IO_H
#define DISEASEMONITOR_DATA_IO_H


#include <stdlib.h>
#include <string.h>
#include "list_lib.h"
#include "hashTable.h"
#include "structs.h"
#include "diseaseAggregator.h"

#define BUCKET_SIZE 120              /*bucket size for just one entry - minimum is 52*/
#define DISEASE_HT_Entries_NUM 10   /*size of diseaseHashTable*/
#define COUNTRY_HT_Entries_NUM 10   /*size of countryHashTable*/
#define BUFFER_SIZE 52              /*minimum buffer size for reading over pipes*/

typedef struct FileDiseaseStats FileDiseaseStats;

enum defAttribute{
    LINE_LENGTH,
    LINE_BUFFER_SIZE,
};

PatientCase* getPatient(char* buffer, FileExplorer* fileExplorer, int fileExplorerPointer);

FILE* openFile(char *inputFile);

MonitorInputArguments* getMonitorInputArgs(int argc, char** argv);

int getMaxFromFile(FILE* patientRecordsFile, int returnVal);

bool writeEntry(char* buffer, List* patientList, HashTable* diseaseHashTable, HashTable* countryHashTable, int bucketSize, int fileExplorerPointer);

CmdManager* read_input_file(FILE* patientRecordsFile, size_t maxStrLength, CmdManager* cmdManager, FileExplorer* fileExplorer, int fileExplorerPointer, bool signalServerToreadStats);

bool dateInputValidation(Date* entryDate, Date* exitDate);

bool setDate(PatientCase *patient, char *buffer);

CmdManager* initializeStructures(MonitorInputArguments *monitorInputArguments);

void deallockFileExplorer(FileExplorer *fileExplorer);

CmdManager* read_directory_list(CmdManager* cmdManager);

int compare (const void * a, const void * b);

FileDiseaseStats** getFileStats(CmdManager* manager, char* country, Date * date);

void deallockFileDiseaseStats(FileDiseaseStats* fileDiseaseStats);

#endif //DISEASEMONITOR_DATA_IO_H
