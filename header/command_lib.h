//
// Created by AriannaK97 on 12/3/20.
//

#ifndef DISEASEMONITOR_COMMAND_LIB_H
#define DISEASEMONITOR_COMMAND_LIB_H

#include "diseaseAggregator.h"

#define MAXIMUM(x, y) (((x) > (y)) ? (x) : (y))

typedef struct CmdManager CmdManager;
typedef struct Date Date;

void listCountries(AggregatorServerManager* aggregatorServerManager);

void diseaseFrequency(CmdManager* manager, char* virusName, Date* date1, Date* date2, char* country);

void topk_AgeRanges(CmdManager* manager, int k, char* country, char* disease , Date* date1, Date* date2);

void searchPatientRecord(CmdManager* manager, char* recordID);

void numPatientAdmissions(CmdManager* manager, char* disease, Date* date1, Date* date2, char* country);

void numPatientDischarges(CmdManager* manager, char* disease, Date* date1, Date* date2, char* country);

void exitMonitor(CmdManager* manager);

int compareTopkAgeRanges (const void * a, const void * b);

void helpDesc();

#endif //DISEASEMONITOR_COMMAND_LIB_H
