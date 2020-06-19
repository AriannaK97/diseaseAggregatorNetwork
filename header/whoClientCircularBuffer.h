//
// Created by AriannaK97 on 18/6/20.
//

#ifndef WHOCLIENT_WHOCLIENTCIRCULARBUFFER_H
#define WHOCLIENT_WHOCLIENTCIRCULARBUFFER_H
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#define QUERY_LENGTH 224

/**
 * Structures
 * */

typedef struct QueryItem{
    char* query;
    int type;
}QueryItem;

typedef struct CircularBuffer{
    char** queries;
    size_t head;
    size_t tail;
    size_t max; //of the buffer
    bool full;
}CircularBuffer;


/**
 * Function Declaration
 * */

CircularBuffer* circularBufInit(size_t size);

void circularBufFree(CircularBuffer *cbuf);

void circularBufReset(CircularBuffer *cbuf);

void circularBufPut(CircularBuffer *cbuf, char* data);

char* circularBufGet(CircularBuffer *cbuf);

bool circularBufEmpty(CircularBuffer *cbuf);

bool circularBufFull(CircularBuffer *cbuf);

size_t circularBufCapacity(CircularBuffer *cbuf);

void advancePointer(CircularBuffer *cbuf);

void retreatPointer(CircularBuffer *cbuf);

size_t circularBufSize(CircularBuffer *cbuf);


#endif //WHOCLIENT_WHOCLIENTCIRCULARBUFFER_H
