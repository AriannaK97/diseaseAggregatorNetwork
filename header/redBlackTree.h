//
// Created by AriannaK97 on 14/3/20.
//

#ifndef DISEASEMONITOR_REDBLACKTREE_H
#define DISEASEMONITOR_REDBLACKTREE_H

#include <stdbool.h>
#include <stdlib.h>
#include "structs.h"
#include "redBlackTree.h"

typedef struct rbNode{
    int colour;
    struct Date* key;
    struct Node* listNodeEntry;
    struct rbNode* left;
    struct rbNode* right;
    struct rbNode* parent;
}rbNode;

typedef struct rbTree{
    struct rbNode* root;
    struct rbNode* nil;
}rbTree;


enum colours{
    Red,
    Black,
};

rbTree* createRbTree();

rbNode* createRbTreeNode(void* listNode);

void leftRotate(rbTree* rbTree, rbNode* treeNode);

void rightRotate(rbTree* tree, rbNode* x);

void rbInsert(rbTree* tree, rbNode* z);

void rbInsertFixup(rbTree* tree, rbNode* z);

void* getKey(Node* listNode);

rbNode* searchRbNode(rbTree* tree, void* key);

void printRbTree(rbNode* root, int depth);

int compare_dates (Date* d1, Date* d2);

rbNode* searchRbNodeRec(rbNode* root, rbNode* nil, void* key);

void freeRbNodesRec(rbTree* tree, rbNode* node);

void freeRbTree(rbTree* tree);

int rbNodeCounter(rbNode* root, rbNode* nil, int operationCall, HashElement* hashIterator);

int countPatients(rbTree* tree, int operationCall, HashElement* hashIterator);

int rbNodeCounter_BetweenDates(rbNode* root, rbNode* nil, int operationCall, HashElement* hashIterator);

bool checkDateSpace(PatientCase* patient, Date* date1, Date* date2);

int countPatients_BetweenDates(rbTree* tree, int operationCall, HashElement* hashIterator);

#endif //DISEASEMONITOR_REDBLACKTREE_H
