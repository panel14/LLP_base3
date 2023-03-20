#include "dataFile.h"

static uint64_t max(uint64_t a, uint64_t b) {
	return (a > b) ? a : b;
}

static enum readStatus readAttribute(struct nodeAttributeInfo* nodeAttr, FILE* fp) {
	struct attributeHeader* header = malloc(ATTR_HEAD_SIZE);
	enum readStatus ret = readFile(fp, header, ATTR_HEAD_SIZE);
	nodeAttr->header = header;

	char* attributeName = (char*)malloc(header->size / FILE_GRANULARITY + (header->size % FILE_GRANULARITY ? FILE_GRANULARITY : 0));
	ret |= readFile(fp, attributeName, header->size);
	nodeAttr->attributeName = attributeName;

	return ret;
}

static enum readStatus readTreeMeta(struct treeMeta* meta, FILE* fp) {
	return readFile(fp, meta, META_SIZE);
}

static size_t getStringLenght(FILE* fp, uint64_t offset) {
	fseek(fp, offset, SEEK_SET);
	size_t lenght = 1;
	union nodeHeader* header = malloc(NODE_HEAD_SIZE);

	readFile(fp, header, NODE_HEAD_SIZE);

	while (header->nextNode) {
		fseek(fp, header->nextNode, SEEK_SET);
		readFile(fp, header, NODE_HEAD_SIZE);
		lenght++;
	}

	free(header);
	return lenght;
}

size_t getRealNodeSize(uint64_t size) {
	return size * SINGLE_NODE_SIZE < MIN_NODE_SIZE
		? MIN_NODE_SIZE
		: size * SINGLE_NODE_SIZE;
}

size_t getIdArraySize(uint64_t attrSize, uint64_t curId) {
	size_t realNodeSize = getRealNodeSize(attrSize) + NODE_HEAD_SIZE;
	if (curId == 0) curId++;

	size_t whole = (curId * OFFSET / realNodeSize);
	size_t frac = (curId * OFFSET % realNodeSize ? 1 : 0);
	size_t value = max((frac + whole) * realNodeSize / OFFSET, MIN_ID_ARRAY_SIZE * realNodeSize / OFFSET);

	return value;
}

enum readStatus readTreeSchema(struct treeSchema* schema, FILE* fp) {
	fseek(fp, 0, SEEK_SET);
	
	struct treeMeta* meta = malloc(META_SIZE);
	enum readStatus ret = readTreeMeta(meta, fp);
	schema->meta = meta;

	struct nodeAttributeInfo** array_of_key = malloc(sizeof(struct nodeAttributeInfo*) * meta->templateSize);
	schema->nodesTemplate = array_of_key;

	for (size_t iter = meta->templateSize; iter-- > 0; array_of_key++) {
		struct nodeAttributeInfo* info = malloc(ATTR_INFO_SIZE);
		ret |= readAttribute(info, fp);
		*array_of_key = info;
	}

	size_t realIdArraySize = getIdArraySize(schema->meta->templateSize, schema->meta->curId);
	uint64_t* idArray = (uint64_t*)malloc(realIdArraySize * sizeof(uint64_t));
	schema->root = idArray;
	ret |= readFile(fp, idArray, realIdArraySize * sizeof(uint64_t));

	return ret;
}

enum readStatus readNode(FILE* fp, struct keyNode** node, uint64_t size, enum nodeType type) {
	union nodeHeader* header = malloc(NODE_HEAD_SIZE);
	enum readStatus ret = readFile(fp, header, NODE_HEAD_SIZE);

	struct keyNode* tmp = malloc(sizeof(struct keyNode));
	tmp->header = *header;
	free(header);

	size_t realSize = getRealNodeSize(size);

	uint64_t* data;
	if (type == NODE_TYPE_USUAL) data = malloc(getRealNodeSize(realSize));
	else data = (uint64_t*)malloc(getRealNodeSize(realSize));

	ret |= readFile(fp, data, getRealNodeSize(realSize));
	tmp->data = data;

	*node = tmp;

	return ret;
}

enum readStatus readStringFromNode(FILE* fp, char** string, uint64_t size, uint64_t offset) {
	size_t strLen = getStringLenght(fp, offset);
	size_t realNodeSize = getRealNodeSize(size);

	*string = malloc(strLen * realNodeSize);

	struct keyNode* tmpNode;

	for (size_t i = 0; i < strLen; i++) {
		fseek(fp, offset, SEEK_SET);
		readNode(fp, &tmpNode, size, NODE_TYPE_STRING);
		offset = tmpNode->header.nextNode;
		strncpy((*string) + realNodeSize * i, (char*)tmpNode->data, realNodeSize);
		freeNode(tmpNode);
	}
	return 0;
}

enum writeStatus writeTreeMeta(FILE* fp, struct treeMeta* meta) {
	enum writeStatus ret = writeFile(fp, meta, META_SIZE);
	return ret;
}

enum writeStatus writeAttributesTemplate(FILE* fp, struct nodeAttributeInfo** template, size_t size) {
	enum writeStatus ret = WRITE_SUCCESS;
	for (; size-- > 0; template++) {
		ret |= writeFile(fp, (*template)->header, ATTR_HEAD_SIZE);
		ret |= writeFile(fp, (*template)->attributeName, (*template)->header->size);
	}

	return ret;
}

enum writeStatus writeRoot(FILE* fp, uint64_t* seq, size_t size) {
	enum writeStatus ret = writeFile(fp, seq, size);
	return ret;
}

enum writeStatus writeTreeSchema(FILE* fp, struct treeSchema* schema) {
	fseek(fp, 0, SEEK_SET);
	size_t templateSize = schema->meta->templateSize;

	enum writeStatus ret = writeTreeMeta(fp, schema->meta);

	fseek(fp, META_SIZE, SEEK_SET);
	ret |= writeAttributesTemplate(fp, schema->nodesTemplate, templateSize);

	size_t realSize = getIdArraySize(schema->meta->templateSize, schema->meta->curId);
	ret |= writeRoot(fp, schema->root, realSize * sizeof(uint64_t));

	return ret;
}

enum writeStatus initEmptyFile(FILE* fp, char** template, uint32_t* types, size_t templateSize, size_t* attrSizes) {
	fseek(fp, 0, SEEK_SET);
	struct treeSchema* schema = malloc(SCHEMA_SIZE);
	createEmptyTreeSchema(template, types, templateSize, attrSizes, schema);

	enum writeStatus ret = writeTreeSchema(fp, schema);
	freeTreeSchema(schema);
	return ret;
}

enum writeStatus writeNode(FILE* fp, struct keyNode* node, size_t size) {
	union nodeHeader* header = malloc(NODE_HEAD_SIZE);
	*header = node->header;
	enum writeStatus ret = writeFile(fp, header, NODE_HEAD_SIZE);
	free(header);

	ret |= writeFile(fp, node->data, size);
	return ret;
}

enum openStatus openForse(FILE** fp, char* filename) {
	enum openStatus ret = openOrCreateFile(filename, fp, "r+w");
	if (ret) ret = openOrCreateFile(filename, fp, "a+");
	return ret;
}

void freeNode(struct keyNode* node) {
	free(node->data);
	free(node);
}

void freeTreeSchema(struct treeSchema* schema) {
	for (size_t i = 0; i < schema->meta->templateSize; i++) {
		free(schema->nodesTemplate[i]->attributeName);
		free(schema->nodesTemplate[i]->header);
		free(schema->nodesTemplate[i]);
	}

	free(schema->nodesTemplate);
	free(schema->root);
	free(schema->meta);
	free(schema);
}
