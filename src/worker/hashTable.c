//
// Created by AriannaK97 on 12/3/20.
//

#include "../../header/hashTable.h"


unsigned long hash(unsigned long x){
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}

/**
 * Create a hashtable with capacity 'capacity'
 * and return a pointer to it
 */
HashTable* hashCreate(unsigned int capacity){
    HashTable* hTable = malloc(sizeof(HashTable));
    if(!hTable){
        return NULL;
    }
    if((hTable->table = malloc(capacity*sizeof(Bucket*))) == NULL){
        free(hTable->table);
        return NULL;
    }
    hTable->capacity = capacity;
    hTable->e_num = 0;
    unsigned int i;
    for(i = 0; i < capacity; i++){
        hTable->table[i] = NULL;
    }
    return hTable;
}

void freeHashTable(HashTable* hTable){
    Bucket* bucket = NULL;
    Bucket* prevBucket = NULL;
    for (unsigned int i = 0; i < hTable->capacity; i++){
        bucket = hTable->table[i];
        while(bucket != NULL){
            unsigned int freeSpace = bucket->bucketSize - sizeof(Bucket*) - sizeof(int) - sizeof(size_t);
            unsigned int entries = freeSpace/DATA_SPACE;
            for(unsigned int j=0; j < entries; j++){
                free(bucket->entry[j].data);
                freeRbTree(bucket->entry[j].tree);
            }free(bucket->entry);
            prevBucket = bucket;
            bucket = bucket->next;
            free(prevBucket);
        }
    }
    free(hTable->table);
    free(hTable);
}


bool bucketHasSpace(Bucket *bucket){
    //the new data can fit in the existing bucket
    unsigned int freeSpace = bucket->bucketSize - sizeof(Bucket*) - sizeof(int) - sizeof(size_t);
    unsigned int entries = freeSpace/DATA_SPACE;
    if(bucket->numOfEntries < entries){
        return true;
    }else{
        return false;
    }
}

/**
 * When the previous bucket is full and only.
 * */
Bucket* getBucket(size_t bucketSize, Bucket *prevBucket){
    Bucket *e;
    e = malloc(sizeof(Bucket));
    if(e == NULL){
        return NULL;
    }

    unsigned int freeSpace = bucketSize - sizeof(Bucket*) - sizeof(int) - sizeof(size_t);
    unsigned int entries = freeSpace/DATA_SPACE;
    e->entry = malloc(entries * sizeof(BucketEntry));
    e->bucketSize = prevBucket->bucketSize;
    e->numOfEntries = 0;
    prevBucket->next = e;
    e->next = NULL;

    return e;
}


/**
 * Store data in the hashtable. If data with the same key are already stored,
 * they are overwritten, and return by the function. Else it return NULL.
 * Return hashERROR if there are memory alloc error
 */
void* hashPut(HashTable* hTable, unsigned long key, void* data, size_t bucketSize, Node* listNode){
    if(data == NULL){
        return NULL;
    }
    unsigned int h = hash(key) % hTable->capacity;
    Bucket* bucket = NULL;
    bucket = hTable->table[h];

    if(bucket != NULL){
        putInBucketData(bucket, bucketSize, data, hTable, key, listNode);
        return NULL;
    }

    // Getting here means the key doesn't already exist
    bucket = malloc(sizeof(Bucket));
    bucket->next = NULL;
    bucket->bucketSize = bucketSize;
    bucket->numOfEntries = 0;
    unsigned int freeSpace = bucket->bucketSize - sizeof(Bucket*) - sizeof(int) - sizeof(size_t);
    unsigned int entries = freeSpace/DATA_SPACE;

    bucket->entry = malloc(entries * sizeof(BucketEntry));
    for(unsigned int i = 0; i < entries; i++){
        bucket->entry[i].data = malloc(DATA_SPACE* sizeof(char));
        bucket->entry[i].tree = (struct rbTree *) createRbTree();
        bucket->entry[i].key = 0;
    }

    memcpy(bucket->entry[0].data, data, DATA_SPACE);
    bucket->entry[0].key = key;
    //create the rbtree for the new entry
    rbNode* treeNode = createRbTreeNode(listNode);
    rbInsert((rbTree*)bucket->entry[0].tree, treeNode);
    bucket->numOfEntries+=1;

    // Add the element at the beginning of the linked list
    bucket->next = hTable->table[h];
    hTable->table[h] = bucket;
    hTable->e_num ++;

    return NULL;
}



/**
 * either the virus or the country occupy 32bytes
 * */
void putInBucketData(Bucket* bucket, size_t bucketSize, char* data, HashTable* hTable, unsigned long key, Node* listNode){
    int bCounter = 0;
    while (bucket != NULL){

        for(int i = 0; i < bucket->numOfEntries; i++){
            if(strcmp(data, bucket->entry[i].data)==0){
                rbNode* treeNode = createRbTreeNode(listNode);
                rbInsert((rbTree*)bucket->entry[i].tree, treeNode);
                return;
            }
        }

        //we have the final bucket of the list and have not found any matches previously
        if(bucket->next == NULL){

            if(bucketHasSpace(bucket)){

                memcpy(bucket->entry[bucket->numOfEntries].data, data, DATA_SPACE);
                bucket->entry[bucket->numOfEntries].key = key;
                //create the rbtree for the new entry
                rbNode* treeNode = createRbTreeNode(listNode);
                rbInsert((rbTree*)bucket->entry[bucket->numOfEntries].tree, treeNode);

                bucket->numOfEntries+=1;
                hTable->e_num ++;
                return;

            }else{

                unsigned int freeSpace = bucketSize - sizeof(Bucket*) - sizeof(int) - sizeof(size_t);
                unsigned int entries = freeSpace/DATA_SPACE;

                bucket = getBucket(bucketSize, bucket);

                for(unsigned int i = 0; i < entries; i++){
                    bucket->entry[i].data = malloc(DATA_SPACE* sizeof(char));
                    bucket->entry[i].tree = createRbTree();
                    bucket->entry[i].key = 0;
                }

                memcpy(bucket->entry[0].data, data, DATA_SPACE);
                bucket->entry[0].key = key;
                //create the rbtree for the new entry
                rbNode* treeNode = createRbTreeNode(listNode);
                rbInsert((rbTree*)bucket->entry[0].tree, treeNode);

                bucket->numOfEntries+=1;
                hTable->e_num ++;

                return;
            }
        }

        bucket = bucket->next;
        bCounter++;
    }
}

/**
 * Retrieve data from the hashtable
 */
void* hashGet(HashTable* hTable, unsigned long key){
    unsigned int h = hash(key) % hTable->capacity;
    Bucket* bucket = hTable->table[h];
    while(bucket != NULL){
        for (int i = 0; i < bucket->numOfEntries; i++)
            if(bucket->entry[i].key == key) //cover collision cases
                return bucket->entry[i].data;
        bucket = bucket->next;
    }
    return NULL;
}

/**
 * Iterate over buckets Depending on the operationCall Value
 * */
int iterateBucketData(Bucket* bucket, int operationCall, HashElement* hashIterator){
    BucketEntry *iterator = bucket->entry;
    int counter = 0;
    Node* listNode;

    for(int i = 0; i < bucket->numOfEntries; i++){
        if(iterator[i].tree != NULL){
            if(operationCall == SEARCH){
                searchRbNode((rbTree*)iterator[i].tree, hashIterator->date1);

            }else if (operationCall == COUNT_ALL_BETWEEN_DATES){

                counter += countPatients_BetweenDates((rbTree*)iterator[i].tree, operationCall, hashIterator);
                if(strlen(iterator[i].data)!=0)
                    fprintf(stdout,"%s %d\n",iterator[i].data, counter);
                    //fprintf(stdout,"The number of patients monitored for %s: %d\n",iterator[i].data, counter);
                hashIterator->counter += counter;
                counter = 0;

            }else if(operationCall == COUNT_ALL_BETWEEN_DATES_WITH_VIRUS
                     || operationCall == COUNT_ALL_BETWEEN_DATES_WITH_VIRUS_AND_COUNTRY){

                if(strcmp(iterator[i].data, hashIterator->virus)==0){
                    counter += countPatients_BetweenDates((rbTree*)iterator[i].tree, operationCall, hashIterator);
/*                    if(strlen(iterator[i].data)!=0)
                        fprintf(stdout,"%s %d\n",iterator[i].data, counter);*/
                        //fprintf(stdout,"The number of patients monitored for %s: %d\n",iterator[i].data, counter);
                    hashIterator->counter += counter;
                    counter = 0;
                }

            }else if (operationCall == GET_HEAP_NODES_AGE_RANGE_DATES){

                if(strcmp(iterator[i].data, hashIterator->virus)==0){
                    counter += countPatients_BetweenDates((rbTree*)iterator[i].tree, operationCall, hashIterator);
                }

            }else if(operationCall == COUNT_DISEASES){
                counter += countPatients((rbTree *) iterator[i].tree, operationCall, hashIterator);
                if(counter > 0){
                    if(hashIterator->DiseaseList == NULL){
                        DiseaseNode* newNode = createDiseaseNode(iterator[i].data);
                        listNode = nodeInit(newNode);
                        hashIterator->DiseaseList = linkedListInit(listNode);
                        hashIterator->DiseaseList->itemCount++;
                    }else{
                        DiseaseNode* newNode = createDiseaseNode(iterator[i].data);
                        listNode = nodeInit(newNode);
                        push(listNode, hashIterator->DiseaseList);
                        hashIterator->DiseaseList->itemCount++;
                    }
                    hashIterator->counter += 1;
                    counter = 0;
                }
            }else if (operationCall == GET_FILE_STATS){
                    counter += countPatients((rbTree *) iterator[i].tree, operationCall, hashIterator);
            }else if(operationCall == COUNT_HOSPITALISED || operationCall == COUNT_ALL){
                counter += countPatients((rbTree *) iterator[i].tree, operationCall,NULL);
                if(strlen(iterator[i].data)!=0)
                    fprintf(stdout, "%s %d\n", iterator[i].data, counter);
                    //fprintf(stdout, "The number of patients monitored for %s: %d\n", iterator[i].data, counter);
                hashIterator->counter += counter;
                counter = 0;

            }else if(operationCall == PRINT) {
                if(strlen(iterator[i].data)!=0)
                    fprintf(stdout, "%s\n", iterator[i].data);

            }
        }
    }
    return counter;
}

/**
 * Iterate through table's elements.
 */
Bucket* hashIterate(HashElement* iterator, int operationCall){
    while(iterator->elem == NULL){
        if(iterator->index < iterator->ht->capacity - 1){
            iterator->index++;
            iterator->elem = iterator->ht->table[iterator->index];
        }
        else{
            return NULL;
        }
    }
    Bucket* bucket = iterator->elem;
    if(bucket){
        iterateBucketData(bucket, operationCall, iterator);
        iterator->elem = bucket->next;
    }
    return bucket;
}

/**
 * Iterate through values.
 */
void* hashIterateValues(HashElement* iterator, int operationCall){
    Bucket* e = hashIterate(iterator, operationCall);
    return (e == NULL ? NULL : e->entry->data);
}

/**
 * Function used for recursive iterations over the hashtable's data
 * According to the operationCall given it will execute the code part needed
 * for the incoming query - the function is used for query implementation
 * */
void applyOperationOnHashTable(HashTable* hTable, int operationCall){
    HashElement iterator = hashITERATOR(hTable);
    while(hashIterateValues(&iterator, operationCall) != NULL);
}