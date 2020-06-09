//
// Created by AriannaK97 on 10/3/20.
//
#include "../../header/list_lib.h"


Node* nodeInit(void* item){

    Node* newNode =  (struct Node*)malloc(sizeof(Node));

    newNode->item = item;
    newNode->next = NULL;

    return newNode;
}

List* linkedListInit(Node* node){
    List* newList = (List*)calloc(sizeof(List), 1);
    newList->head = node;
    newList->tail = node;
    newList->itemCount = 1;
    return newList;
}

void push(Node* listNode, List* linkedList){

    if(linkedList->head == NULL){
        linkedList->head = listNode;
        linkedList->tail = listNode;
        linkedList->itemCount += 1;
    }else{
        linkedList->tail->next = listNode;
        linkedList->tail = listNode;
        linkedList->itemCount += 1;
    }
}

/**
 * List memory deallocation for auxiliary list in heap
 * */
void iteratorListMemoryDeallock(List* linkedList){
    Node* listNode = linkedList->head;
    while(listNode != NULL){
        linkedList->head = linkedList->head->next;
        free(listNode);
        listNode = linkedList->head;
    }
    free(linkedList);
}

/**
 * List memory deallocation
 * */
void patientListMemoryDeallock(List* linkedList){
    Node* listNode = linkedList->head;
    while(listNode != NULL){
        linkedList->head = linkedList->head->next;
        nodeItemDeallock(listNode->item);
        free(listNode);
        listNode = linkedList->head;
    }
    free(linkedList);
}

void nodeItemDeallock(PatientCase* item){
    free(item->entryDate);
    free(item->exitDate);
    free(item->name);
    free(item->surname);
    free(item->country);
    free(item->virus);
    free(item->recordID);
    free(item->type);
    free(item);
}

Node* popNode(List* linkedList){
    Node* node = linkedList->head;
    linkedList->head = linkedList->head->next;
    if(linkedList->head == NULL){
        linkedList->tail = NULL;
    }
    return node;
}

bool compareListItemPatient(PatientCase* patient, char* key){
    if (strcmp(patient->recordID, key) == 0){
        return true;
    }
    return false;
}

bool searchNodeForRecordID_ExitDateUpdate(List* linkedList, char* key, Date* exitDate){
    Node* node = linkedList->head;
    while (node != NULL){
        if (compareListItemPatient(node->item, key)){
            PatientCase* exitPatient = (PatientCase*)node->item;
            if(dateInputValidation(exitPatient->entryDate, exitDate)){
                exitPatient->exitDate->day = exitDate->day;
                exitPatient->exitDate->month = exitDate->month;
                exitPatient->exitDate->year = exitDate->year;
                return true;
            }else{
                //fprintf(stderr, "Error\n");
/*                fprintf(stdout,"Exit date could not be updated for patient:\n case number: %s | name: %s | "
                               "surname: %s | virus: %s | country: %s | entryDate: %d-%d-%d\n\nInvalid exit date: "
                               "%d-%d-%d", exitPatient->recordID, exitPatient->name, exitPatient->surname,
                               exitPatient->virus, exitPatient->country, exitPatient->entryDate->day,
                               exitPatient->entryDate->month, exitPatient->entryDate->year, exitPatient->exitDate->day,
                               exitPatient->exitDate->month, exitPatient->exitDate->year);*/
                return false;
            }
        }
        node = node->next;
    }
    //fprintf(stderr, "Could not find the patient with record id %s in the system\n", key);
    //fprintf(stderr, "Error\n");
    return false;
}

bool updateListVirusSum(List* linkedList, int key, char* disease){
    Node* node = linkedList->head;
    AgeRangeStruct* currentContent;
    while (node != NULL){
        currentContent = node->item;
        if (currentContent->data <= 20 && key <= 20 && strcmp(currentContent->disease, disease) == 0){
            currentContent->dataSum++;
            return true;
        }else if(currentContent->data <= 40 && key <= 40 && strcmp(currentContent->disease, disease) == 0){
            currentContent->dataSum++;
            return true;
        }else if(currentContent->data <= 60 && key <= 60 && strcmp(currentContent->disease, disease) == 0){
            currentContent->dataSum++;
            return true;
        }else if(currentContent->data <= 120 && key <=120 && strcmp(currentContent->disease, disease) == 0){
            currentContent->dataSum++;
            return true;
        }
        node = node->next;
    }
    return false;
}

AgeRangeStruct* createAgeRangeNode(int data, int dataSum, char* disease){
    AgeRangeStruct* ageRangeNode =  malloc(sizeof(AgeRangeStruct));
    ageRangeNode->disease = malloc(sizeof(char)*DATA_SPACE);
    strcpy(ageRangeNode->disease, disease);
    ageRangeNode->dataSum = dataSum;
    ageRangeNode->data = data;
    return ageRangeNode;
}


void ageRangeNodeDeallock(AgeRangeStruct* ageRangeStruct){
    free(ageRangeStruct->disease);
    free(ageRangeStruct);
}

void diseaseNodeDeallock(DiseaseNode* diseaseNode){
    free(diseaseNode->disease);
    free(diseaseNode);
}

DiseaseNode* createDiseaseNode(char* disease){
    DiseaseNode* diseaseNode = malloc(sizeof(DiseaseNode));
    diseaseNode->disease = malloc(sizeof(char)*strlen(disease)+1);
    strcpy(diseaseNode->disease, disease);
    return diseaseNode;
}


/**
 * Search for duplicates in list
 * Used for new entry validation
 * */
bool searchListForRecordID(List* linkedList, char* key){
    Node* node = linkedList->head;
    while (node != NULL){
        if (compareListItemPatient(node->item, key)){
            fprintf(stderr, "System stopped due to input error.\nThe recordID already exists in the system.\n Exit...\n");
            exit(1);
        }
        node = node->next;
    }
    return false;
}

/**
 * Print list
 * */
void printList(List* patientList){
    PatientCase* newPatient;
    printf("this is patient list\n");
    Node* newNode = patientList->head;
    while (newNode != NULL && newNode->item != NULL){
        newPatient = (PatientCase*)newNode->item;
        fprintf(stdout,"case number: %s | name: %s | surname: %s | virus: %s | country: %s | importDate: %d-%d-%d | "
                       "exportDate: %d-%d-%d\n", newPatient->recordID, newPatient->name, newPatient->surname, newPatient->virus,
                newPatient->country, newPatient->entryDate->day, newPatient->entryDate->month, newPatient->entryDate->year
                , newPatient->exitDate->day, newPatient->exitDate->month, newPatient->exitDate->year);

        newNode = newNode->next;
    }
}

/**
 * Print a certain given node of type Node
 * */
void printListNode(Node* node){
    if(node == NULL){
        return;
    }
    PatientCase* patient = node->item;
    fprintf(stdout,"case number: %s | name: %s | surname: %s | virus: %s | country: %s | importDate: %d-%d-%d | "
                   "exportDate: %d-%d-%d\n", patient->recordID, patient->name, patient->surname, patient->virus,
            patient->country, patient->entryDate->day, patient->entryDate->month, patient->entryDate->year,
            patient->exitDate->day, patient->exitDate->month, patient->exitDate->year);
}


PatientCase* getPatientFromList(List* linkedList, char* recordID){
    Node* node = linkedList->head;
    while (node != NULL){
        if (compareListItemPatient(node->item, recordID)){
            return node->item;
        }
        node = node->next;
    }
    return NULL;
}


void dirListMemoryDeallock(List* linkedList){
    Node* listNode = linkedList->head;
    while(listNode != NULL){
        linkedList->head = linkedList->head->next;
        nodeDirListItemDeallock(listNode->item);
        free(listNode);
        listNode = linkedList->head;
    }
    free(linkedList);
}