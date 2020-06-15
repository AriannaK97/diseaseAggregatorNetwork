//
// Created by AriannaK97 on 14/3/20.
//
#include "../../header/redBlackTree.h"
#include <stdio.h>
#include "../../header/list_lib.h"

void leftRotate(rbTree* tree, rbNode* x){
    rbNode* y = x->right;

    x->right = y->left;
    if(x->right != tree->nil)
        x->right->parent = x;

    y->parent = x->parent;
    if(y->parent == tree->nil){
        tree->root = y;
    }else if(x == x->parent->left){
        x->parent->left = y;
    }else{
        x->parent->right = y;
    }
    y->left = x;
    x->parent = y;
}

void rightRotate(rbTree* tree, rbNode* x){
    rbNode* y = x->left;

    x->left = y->right;
    if(x->left != tree->nil)
        x->left->parent = x;

    y->parent = x->parent;

    if(y->parent == tree->nil){
        tree->root = y;
    }else if(x == x->parent->left){
        x->parent->left = y;
    }else{
        x->parent->right = y;
    }
    y->right = x;
    x->parent = y;
}
/**
 * If the first argument is greater than the second it returns true
 * else it return false.
 * */

int compare_dates (Date* d1, Date* d2){
    if(d1 == NULL || d2 == NULL)
        return 3;

    if (d1->year < d2->year)
        return -1;

    else if (d1->year > d2->year)
        return 1;

    if (d1->year == d2->year)
    {
        if (d1->month<d2->month)
            return -1;
        else if (d1->month>d2->month)
            return 1;
        else{
            if (d1->day<d2->day)
                return -1;
            else if(d1->day>d2->day)
                return 1;
            else
                return 0;
        }
    }
    return 3;
}

rbTree* createRbTree(){
    rbTree* tree = malloc(sizeof(rbTree));
    tree->nil = malloc(sizeof(rbNode));

    tree->nil->parent = NULL;
    tree->nil->key = NULL;
    tree->nil->listNodeEntry = NULL;
    tree->nil->colour = Black;
    tree->nil->left = tree->nil;
    tree->nil->right = tree->nil;
    tree->root = tree->nil;

    return tree;
}


void* getKey(Node* listNode){
    Date* treeDate;
    PatientCase* patient = listNode->item;
    treeDate = patient->entryDate;
    return treeDate;
}

rbNode* createRbTreeNode(void* listNode){
    rbNode* treeNode = malloc(sizeof(rbNode));

    treeNode->key = getKey(listNode);
    treeNode->listNodeEntry = listNode;
    treeNode->colour =  Red;
    treeNode->parent = NULL;
    treeNode->right = NULL;
    treeNode->left =NULL;

    return treeNode;
}

void rbInsert(rbTree* tree, rbNode* z){
    rbNode* y = tree->nil;
    rbNode* x = tree->root;

    z->left = tree->nil;
    z->right = tree->nil;

    while (x != tree->nil) {
        y = x;
        if (compare_dates(z->key, x->key) == -1)
            x = x->left;
        else
            x = x->right;
    }
    z->parent = y;

    if(y == tree->nil)
        tree->root = z;
    else if(compare_dates(z->key, y->key) == -1)
        y->left = z;
    else
        y->right = z;


    z->colour = Red;
    rbInsertFixup(tree, z);
}

void rbInsertFixup(rbTree* tree, rbNode* z){
    rbNode* y;
    while(z->parent->colour == Red){
        if(z->parent == z->parent->parent->left){
            y = z->parent->parent->right;
            if(y->colour == Red){
                z->parent->colour = Black;
                y->colour = Black;
                z->parent->parent->colour = Red;
                z = z->parent->parent;
            }
            else{
                if(z == z->parent->right){
                    z = z->parent;
                    leftRotate(tree, z);
                }
                z->parent->colour = Black;
                z->parent->parent->colour = Red;
                rightRotate(tree, z->parent->parent);
            }
        }
        else{
            y = z->parent->parent->left;
            if(y->colour == Red){
                z->parent->colour = Black;
                y->colour = Black;
                z->parent->parent->colour = Red;
                z = z->parent->parent;
            }
            else{
                if(z == z->parent->left){
                    z = z->parent;
                    rightRotate(tree, z);
                }
                z->parent->colour = Black;
                z->parent->parent->colour = Red;
                leftRotate(tree, z->parent->parent);
            }
            if(z == tree->root){
                break;
            }
        }

    }
    tree->root->colour = Black;
}


void printRbTree(rbNode* root, int depth){

    if(root->key == NULL){
        return;
    }
    printRbTree(root->left, depth+1);
    PatientCase* patient;
    patient = root->listNodeEntry->item;
    fprintf(stdout,"depth = %d, case number: %s | name: %s | surname: %s | virus: %s | country: %s | importDate: %d-%d-%d | "
                   "exportDate: %d-%d-%d\n", depth, patient->recordID, patient->name, patient->surname, patient->virus,
            patient->country, patient->entryDate->day, patient->entryDate->month, patient->entryDate->year,
            patient->exitDate->day, patient->exitDate->month, patient->exitDate->year);
    printRbTree(root->right, depth+1);

}

int countPatients(rbTree* tree, int operationCall, HashElement* hashIterator){
    return rbNodeCounter(tree->root, tree->nil, operationCall, hashIterator);
}

bool checkDateSpace(PatientCase* patient, Date* date1, Date* date2){
    if (compare_dates(patient->entryDate, date1) >=0 && compare_dates(patient->exitDate, date2) <= 0)
        return true;
    return false;
}

bool checkExitDateSpace(PatientCase* patient, Date* date1, Date* date2){

    if(patient->exitDate->day == 0){
        return false;
    }

    if (compare_dates(patient->exitDate, date1) >=0 && compare_dates(patient->exitDate, date2) <= 0)
        return true;
    return false;
}

int countPatients_BetweenDates(rbTree* tree, int operationCall, HashElement* hashIterator){
    return rbNodeCounter_BetweenDates(tree->root, tree->nil, operationCall, hashIterator);
}

int rbNodeCounter_BetweenDates(rbNode* root, rbNode* nil, int operationCall, HashElement* hashIterator){
    if(root == NULL || root == nil){
        return 0;
    }

    int counter = 0;
    Node* listNode;

    counter += rbNodeCounter_BetweenDates(root->left, nil, operationCall, hashIterator);
    PatientCase* patient = root->listNodeEntry->item;

    if(operationCall == COUNT_ALL_BETWEEN_DATES || operationCall == COUNT_ALL_BETWEEN_DATES_WITH_VIRUS){

        if(checkDateSpace(patient, hashIterator->date1, hashIterator->date2))
            counter++;

    }else if(operationCall == COUNT_HOSPITALISED_BETWEEN_DATES_WITH_DISEASE) {
        if(checkDateSpace(patient, hashIterator->date1, hashIterator->date2)
           && strcmp(patient->virus, hashIterator->virus) == 0){
            counter++;
        }
    } else if(operationCall == COUNT_ALL_BETWEEN_DATES_WITH_VIRUS_AND_COUNTRY){
        if(checkDateSpace(patient, hashIterator->date1, hashIterator->date2)
           && strcmp(patient->country, hashIterator->country) == 0){
            counter++;
        }
    }else if (operationCall == COUNT_HOSPITALISED_BETWEEN_DATES_WITH_DISEASE_AND_COUNTRY){

        if(checkDateSpace(patient, hashIterator->date1, hashIterator->date2)
            && strcmp(patient->virus, hashIterator->virus) == 0){
            counter++;
        }

    } else if (operationCall == COUNT_HOSPITALISED_BETWEEN_DATES_WITH_DISEASE_EXIT){
        if(checkExitDateSpace(patient, hashIterator->date1, hashIterator->date2)
           && strcmp(patient->virus, hashIterator->virus) == 0){
            counter++;
        }
    }else if (operationCall == COUNT_HOSPITALISED_BETWEEN_DATES_WITH_DISEASE_AND_COUNTRY_EXIT){
        if(checkExitDateSpace(patient, hashIterator->date1, hashIterator->date2)
           && strcmp(patient->virus, hashIterator->virus) == 0){
            counter++;
        }
    }
    else if(operationCall == GET_HEAP_NODES_AGE_RANGE_DATES){
        if(checkDateSpace(patient, hashIterator->date1, hashIterator->date2)
        && strcmp(patient->country, hashIterator->country) == 0){
            /*create a list o AgeRangeNodes*/
            if(hashIterator->AgeRangeNodes == NULL){
                AgeRangeStruct* newNode = createAgeRangeNode(patient->age, 1, patient->virus);
                listNode = nodeInit(newNode);
                hashIterator->AgeRangeNodes = linkedListInit(listNode);
            }else if(updateListVirusSum(hashIterator->AgeRangeNodes, patient->age, patient->virus) == false){
                AgeRangeStruct* newNode = createAgeRangeNode(patient->age, 1, patient->virus);
                listNode = nodeInit(newNode);
                push(listNode, hashIterator->AgeRangeNodes);
            }
        }
    }
    counter += rbNodeCounter_BetweenDates(root->right, nil, operationCall, hashIterator);

    return counter;
}


int rbNodeCounter(rbNode* root, rbNode* nil, int operationCall, HashElement* hashIterator) {
    if (root == NULL || root == nil) {
        return 0;
    }

    int counter = 0;
    Node* listNode;

    counter += rbNodeCounter(root->left, nil, operationCall, hashIterator);
    PatientCase *patient = root->listNodeEntry->item;
    if (operationCall == COUNT_HOSPITALISED) {
        if (patient->exitDate->year == 0 && patient->exitDate->day == 0 && patient->exitDate->month == 0)
            counter++;
    }else if (operationCall == COUNT_ALL) {
        counter++;
    }else if(operationCall == GET_HEAP_NODES_AGE_RANGE || operationCall == GET_FILE_STATS){
        if((strcmp(patient->country, hashIterator->country) == 0) &&
           ((patient->exitDate->year == hashIterator->date1->year && patient->exitDate->day == hashIterator->date1->day && patient->exitDate->month == hashIterator->date1->month) ||
            (patient->entryDate->year == hashIterator->date1->year && patient->entryDate->day == hashIterator->date1->day && patient->entryDate->month == hashIterator->date1->month))){
            /*create a list o AgeRangeNodes - later these nodes will be used to populate
             * a size 4 array of AgeRangeNodes
             **/
            if(hashIterator->AgeRangeNodes == NULL){
                AgeRangeStruct* newNode = createAgeRangeNode(patient->age, 1, patient->virus);
                listNode = nodeInit(newNode);
                hashIterator->AgeRangeNodes = linkedListInit(listNode);
            }else if(updateListVirusSum(hashIterator->AgeRangeNodes, patient->age, patient->virus) == false){
                AgeRangeStruct* newNode = createAgeRangeNode(patient->age, 1, patient->virus);
                listNode = nodeInit(newNode);
                push(listNode, hashIterator->AgeRangeNodes);
            }
        }
    }else if(operationCall == COUNT_DISEASES){
        if((strcmp(patient->country, hashIterator->country) == 0) &&
        ((patient->exitDate->year == hashIterator->date1->year && patient->exitDate->day == hashIterator->date1->day && patient->exitDate->month == hashIterator->date1->month) ||
        (patient->entryDate->year == hashIterator->date1->year && patient->entryDate->day == hashIterator->date1->day && patient->entryDate->month == hashIterator->date1->month))){
            counter++;
        }
    }
    counter += rbNodeCounter(root->right, nil, operationCall, hashIterator);

    return counter;
}


rbNode* searchRbNodeRec(rbNode* root, rbNode* nil, void* key){
    if(root == NULL || root == nil){
        return NULL;
    }

    if(compare_dates(root->key, key) == 1){
        searchRbNodeRec(root->left, nil, key);
    }else if(compare_dates(root->key, key) == -1){
        searchRbNodeRec(root->right, nil, key);
    } else {
        searchRbNodeRec(root->left, nil, key);
        PatientCase* patient = root->listNodeEntry->item;
        fprintf(stdout,
                "case number: %s | name: %s | surname: %s | virus: %s | country: %s | importDate: %d-%d-%d | "
                "exportDate: %d-%d-%d\n", patient->recordID, patient->name, patient->surname, patient->virus,
                patient->country, patient->entryDate->day, patient->entryDate->month, patient->entryDate->year,
                patient->exitDate->day, patient->exitDate->month, patient->exitDate->year);
        searchRbNodeRec(root->right, nil, key);
    }
    return root;

}

rbNode* searchRbNode(rbTree* tree, void* key){
    return searchRbNodeRec(tree->root, tree->nil, key);
}

void freeRbTree(rbTree* tree){
    freeRbNodesRec(tree, tree->root);
    free(tree->nil);
    free(tree);
    //fprintf(stdout, "Red Black Tree destroyed successfully!\n");
}

void freeRbNodesRec(rbTree* tree, rbNode* node){
    if(node != tree->nil){
        freeRbNodesRec(tree, node->left);
        freeRbNodesRec(tree, node->right);
        free(node);
    }
}
