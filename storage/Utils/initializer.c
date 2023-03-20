#include "initializer.h"

static void createEmptyTreeMeta(struct treeMeta* meta, size_t size) {
	meta->ASCIISign = 0xFFFE;
	meta->firstSeq = 0;
	meta->secondSeq = 0;
	meta->templateSize = (uint64_t)size;
	meta->curId = 0;
	meta->rootOffset = 0;
}

static void copyStr(char* src, char* dest, size_t srcSize, size_t destSize) {
	while (destSize-- && srcSize--) *(dest++) = *(src++);
}

static void createEmptyAttrTemplate(struct nodeAttributeInfo** attrs, char** template, uint32_t* types, size_t templateSize, size_t* attrSizes) {
	struct nodeAttributeInfo* tempAttr;
	size_t realSize;

	for (size_t i = templateSize; i-- > 0; attrs++ && template++ && types++ && attrSizes++) {
		tempAttr = malloc(ATTR_INFO_SIZE);
		realSize = (*attrSizes) / FILE_GRANULARITY * FILE_GRANULARITY + ((*attrSizes) % FILE_GRANULARITY ? FILE_GRANULARITY : 0);
		char* appendedString = malloc(sizeof(char) * realSize);
		copyStr(*template, appendedString, *attrSizes, realSize);

		tempAttr->attributeName = appendedString;
		tempAttr->header = malloc(ATTR_HEAD_SIZE);
		tempAttr->header->size = (uint32_t)realSize;
		tempAttr->header->type = *types;
		*attrs = tempAttr;
	}
}

void createEmptyTreeSchema(char** template, uint32_t* types, size_t templateSize, size_t* attrSizes, struct treeSchema* schema) {
	schema->meta = malloc(META_SIZE);
	createEmptyTreeMeta(schema->meta, templateSize);

	schema->nodesTemplate = malloc(sizeof(struct nodeAttributeInfo*) * templateSize);
	createEmptyAttrTemplate(schema->nodesTemplate, template, types, templateSize, attrSizes);

	size_t arrSize = getIdArraySize(schema->meta->templateSize, schema->meta->curId);
	schema->root = malloc(sizeof(uint64_t) * arrSize);

	for (size_t i = 0; i < arrSize; i++) {
		schema->root[i] = 0;
	}
}