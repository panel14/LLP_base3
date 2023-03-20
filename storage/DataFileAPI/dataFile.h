#pragma once

#include <stdlib.h>
#include <string.h>
#include "../tree_node/node.h"
#include "../tree_schema/schema.h"
#include "../OSFileAPI/osFile.h"
#include "../Utils/initializer.h"

//in config module?
#define MIN_NODE_SIZE 256
#define SINGLE_NODE_SIZE 8
#define OFFSET 8
#define MIN_ID_ARRAY_SIZE 1
#define FILE_GRANULARITY 4

enum nodeType {
	NODE_TYPE_USUAL = 0,
	NODE_TYPE_STRING
};

size_t getRealNodeSize(uint64_t size);
size_t getIdArraySize(uint64_t attrSize, uint64_t curId);

enum readStatus readTreeSchema(struct treeSchema* schema, FILE* fp);
enum readStatus readNode(FILE* fp, struct keyNode** node, uint64_t size, enum nodeType type);
enum readStatus readStringFromNode(FILE* fp, char** string, uint64_t size, uint64_t offset);

enum writeStatus writeTreeMeta(FILE* fp, struct treeMeta* meta);
enum writeStatus writeAttributesTemplate(FILE* fp, struct nodeAttributeInfo** template, size_t size);
enum writeStatus writeRoot(FILE* fp, uint64_t* seq, size_t size);
enum writeStatus writeTreeSchema(FILE* fp, struct treeSchema* schema);
enum writeStatus writeNode(FILE* fp, struct keyNode* node, size_t size);

enum writeStatus initEmptyFile(FILE* fp, char** template, uint32_t* types, size_t templateSize, size_t* attrSizes);

//to osFile?
enum openStatus openForse(FILE** fp, char* filename);
//new free module?
void freeNode(struct keyNode* node);
void freeTreeSchema(struct treeSchema* schema);