#pragma once
#include <stdio.h>
#include <inttypes.h>
#include "storageBaseApi.h"

#define RESULT_SET_SIZE sizeof(struct resultSetNode)

struct resultSetNode {
	struct resultSetNode* prev;
	struct keyNode* value;
	uint64_t id;
};

size_t addNode(FILE* fp, uint64_t* values, uint64_t parent);
enum crudStatus getNode(FILE* fp, uint64_t** values, uint64_t id);
enum crudStatus removeNode(FILE* fp, uint64_t id, uint8_t strFlag);
enum crudStatus findByParent(FILE* fp, uint64_t parent, struct resultSetNode** resultSet);
enum crudStatus findByField(FILE* fp, uint64_t fieldNumber, uint64_t* searchStr, struct resultSetNode** resultSet);
enum crudStatus updateNode(FILE* file, uint64_t fieldNumber, uint64_t* newValue, uint64_t id);
void printNodesFrom(FILE* fp);
void printTreeSchema(FILE* fp);

void appendToResultSet(struct keyNode** addedNodes, uint64_t id, struct resultSetNode** resultSet);
enum crudStatus removeNode(FILE* fp, uint64_t id, uint8_t strFlag);
