//
// Created by AriannaK97 on 19/6/20.
//

#ifndef WHOCLIENT_WHOSERVERCIRCULARBUFFER_H
#define WHOCLIENT_WHOSERVERCIRCULARBUFFER_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct FileDescriptor{
    int fd;
    int type;
}FileDescriptor;

typedef struct CircularBuffer{
    FileDescriptor *buffer;
    size_t head;
    size_t tail;
    size_t max; //of the buffer
    bool full;
}CircularBuffer;

/**
 * Circular Buffer Methods
 * */

CircularBuffer* circularBufInit(size_t size);

void circularBufFree(CircularBuffer *cbuf);

void circularBufReset(CircularBuffer *cbuf);

void circularBufPut(CircularBuffer *cbuf, int data, int type);

int circularBufGet(CircularBuffer *cbuf, FileDescriptor * data);

bool circularBufEmpty(CircularBuffer *cbuf);

bool circularBufFull(CircularBuffer *cbuf);

size_t circularBufCapacity(CircularBuffer *cbuf);

void advancePointer(CircularBuffer *cbuf);

void retreatPointer(CircularBuffer *cbuf);

size_t circularBufSize(CircularBuffer *cbuf);


#endif //WHOCLIENT_WHOSERVERCIRCULARBUFFER_H
