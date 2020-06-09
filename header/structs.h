//
// Created by AriannaK97 on 15/3/20.
//

#ifndef DISEASEMONITOR_STRUCTS_H
#define DISEASEMONITOR_STRUCTS_H


typedef struct BucketEntry BucketEntry;
typedef struct Bucket Bucket;
typedef struct HashTable HashTable;
typedef struct HashElement HashElement;
typedef struct FileExplorer FileExplorer;

typedef struct Date{
    int day;
    int month;
    int year;
}Date;

typedef struct PatientCase{
    char* recordID;
    char* type;
    char* name;
    char* surname;
    char* virus;
    char* country;
    int age;
    struct Date* entryDate;
    struct Date* exitDate;
}PatientCase;

typedef struct Node{
    void* item;
    struct Node* next;
}Node;

typedef struct List{
    struct Node* head;
    struct Node* tail;
    int itemCount;
}List;

typedef struct WorkerLog{
    int successes;
    int fails;
}WorkerLog;

typedef struct CmdManager{
    struct List* patientList;
    struct HashTable* diseaseHashTable;
    struct HashTable* countryHashTable;
    struct WorkerInfo* workerInfo;
    struct FileExplorer** fileExplorer;
    struct WorkerLog* workerLog;
    struct List* directoryList;
    size_t bucketSize;
    int numOfDirectories;
    char *input_dir;
    int bufferSize;
    int numOfDiseases; /*auxiliary for statistics collection*/
    int fd_client_w;
    int fd_client_r;
    int workerId;
}CmdManager;

typedef struct AgeRange{
    int index;
    int cases;
}AgeRange;

typedef struct MonitorInputArguments{
    size_t bucketSize;
    int diseaseHashtableNumOfEntries;
    int countryHashTableNumOfEntries;
    size_t bufferSize;
    char *input_dir;
    int workerId;
}MonitorInputArguments;

typedef struct AgeRangeStruct{
    int data;
    int dataSum;
    char* disease;
}AgeRangeStruct;

typedef struct DiseaseNode{
    char* disease;
}DiseaseNode;

/**
 *  Global Variables
 **/

CmdManager* cmdManager;

#endif //DISEASEMONITOR_STRUCTS_H
