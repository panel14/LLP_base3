#pragma once

#include <unistd.h>
#include "storageFileInner.h"
#include "../DataFileAPI/dataFile.h"

enum crudStatus {
	CRUD_SUCCESS = 0,
	CRUD_EOF,
	CRUD_FAIL
};

void getTypes(FILE* fp, uint32_t** types, size_t* size);

enum crudStatus getIdOffset(FILE* fp, uint64_t* id, uint64_t offset);
enum crudStatus deleteLastNode(FILE* fp, size_t nodeSize);
enum crudStatus linkSton(FILE* fp, struct keyNode* node, uint64_t offset);
enum crudStatus swapNode(FILE* fp, uint64_t dest, uint64_t src, size_t nodeSize);
enum crudStatus swapLastNode(FILE* fp, uint64_t dest, size_t nodeSize);
enum crudStatus insertNode(FILE* fp, struct keyNode* node, size_t fullNodeSize, uint64_t* nodePos);
enum crudStatus insertNodeStr(FILE* fp, char* string, size_t nodeSize, uint64_t* strPos);
enum crudStatus removeFromIdArray(FILE* fp, uint64_t id, uint64_t* offset);
enum crudStatus convItoO(FILE* fp, uint64_t id, uint64_t* offset);
enum crudStatus convOtoI(FILE* fp, uint64_t* id, uint64_t offset);
enum crudStatus changeStrNode(FILE* fp, uint64_t offset, char* newStr, uint64_t size);

size_t appendToIdArray(FILE* fp, uint64_t offset);

