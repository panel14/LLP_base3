#include "storageBaseApi.h"

enum crudStatus deleteLastNode(FILE* fp, size_t nodeSize) {
	fseek(fp, nodeSize, SEEK_END);
	int fd = fileno(fp);
	return ftruncate(fd, ftell(fp));
}

void getTypes(FILE* fp, uint32_t** types, size_t* size) {
    fseek(fp, 0, SEEK_SET);
    struct treeSchema* schema = malloc(SCHEMA_SIZE);
    readTreeSchema(schema, fp);
    
    uint32_t* tmpTypes = malloc(schema->meta->templateSize * sizeof(uint32_t));

    for (size_t i = 0; i < schema->meta->templateSize; i++) {
        tmpTypes[i] = schema->nodesTemplate[i]->header->type;
    }

    *types = tmpTypes; 
    *size = schema->meta->templateSize;
    freeTreeSchema(schema);
}


enum crudStatus getIdOffset(FILE* fp, uint64_t* id, uint64_t offset) {
    struct treeSchema* schema = malloc(SCHEMA_SIZE);
    readTreeSchema(schema, fp);

    struct keyNode* node;
    fseek(fp, offset, SEEK_SET);
    readNode(fp, &node, schema->meta->templateSize, NODE_TYPE_USUAL);

    if (schema->root[node->header.allocNode] == offset) {
        *id = node->header.allocNode;
        freeTreeSchema(schema);
        freeNode(node);
        return CRUD_SUCCESS;
    }
    else {
        freeTreeSchema(schema);
        freeNode(node);
        return CRUD_FAIL;
    }
}

enum crudStatus linkSton(FILE* fp, struct keyNode* node, uint64_t offset) {
    uint32_t* types;
    size_t size;
    getTypes(fp, &types, &size);

    struct keyNode* str;

    for (uint64_t i = 0; i < size; i++) {
        if (types[i] == STRING) {
            fseek(fp, node->data[i], SEEK_SET);
            readNode(fp, &str, size, NODE_TYPE_STRING);
            str->header.prevNode = offset;
            fseek(fp, node->data[i], SEEK_SET);
            writeNode(fp, str, size);
            freeNode(str);
        }
    }
    free(types);
    return CRUD_SUCCESS;
}

enum crudStatus swapNode(FILE* fp, uint64_t dest, uint64_t src, size_t nodeSize) {
	uint32_t* types;
	size_t size;
    getTypes(fp, &types, &size);

	if (src != dest) {
		fseek(fp, src, SEEK_SET);
        void* buffer = malloc(nodeSize);
        readFile(fp, buffer, nodeSize);


        fseek(fp, dest, SEEK_SET);
        writeFile(fp, buffer, nodeSize);
        free(buffer);

        fseek(fp, 0, SEEK_SET);
        struct treeSchema* schema = malloc(SCHEMA_SIZE);
        readTreeSchema(schema, fp);
        uint64_t id;
        struct keyNode* node;

        enum crudStatus st = getIdOffset(fp, &id, src);

        if (st == CRUD_FAIL) {

            fseek(fp, src, SEEK_SET);
            readNode(fp, &node, size, NODE_TYPE_STRING);
            union nodeHeader* header = malloc(NODE_HEAD_SIZE);

            if (node->header.nextNode != 0) {
                fseek(fp, node->header.nextNode, SEEK_SET);
                readFile(fp, header, NODE_HEAD_SIZE);
                header->prevNode = dest;
                fseek(fp, node->header.nextNode, SEEK_SET);
                writeFile(fp, header, NODE_HEAD_SIZE);
            }

            fseek(fp, node->header.prevNode, SEEK_SET);
            readFile(fp, header, NODE_HEAD_SIZE);
            if (header->nextNode == src) {
                header->nextNode = dest;
                fseek(fp, node->header.prevNode, SEEK_SET);
                writeFile(fp, header, NODE_HEAD_SIZE);
            }
            else {
                struct keyNode* parent;
                fseek(fp, node->header.prevNode, SEEK_SET);
                readNode(fp, &parent, size, NODE_TYPE_USUAL);
                for (size_t iter = 0; iter < size; iter++) {
                    if (types[iter] == STRING && parent->data[iter] == src) {
                        parent->data[iter] = dest;
                        break;
                    }
                }
                fseek(fp, (node->header.prevNode), SEEK_SET);
                writeNode(fp, parent, getRealNodeSize(size));
                freeNode(parent);
            }
            free(header);
        }
        else
        {
            fseek(fp, src, SEEK_SET);
            readNode(fp, &node, size, NODE_TYPE_USUAL);
            linkSton(fp, node, dest);
            schema->root[id] = dest;
            writeTreeSchema(fp, schema);
        }

        freeNode(node);
        freeTreeSchema(schema);
	}
    free(types);
    return CRUD_SUCCESS;
}

enum crudStatus swapLastNode(FILE* fp, uint64_t dest, size_t nodeSize) {
    uint32_t* types;
    size_t size;
    getTypes(fp, &types, &size);

    fseek(fp, (long) -(getRealNodeSize(size) + NODE_HEAD_SIZE), SEEK_SET);
    uint64_t src = ftell(fp);

    free(types);
    enum crudStatus stat = swapNode(fp, dest, src, nodeSize);
    ftruncate(fileno(fp), (long)src);
    return stat;
}

enum crudStatus insertNode(FILE* fp, struct keyNode* node, size_t fullNodeSize, uint64_t* nodePos) {
    fseek(fp, 0, SEEK_END);
    *nodePos = ftell(fp);

    int fd = fileno(fp);
    ftruncate(fd, ftell(fp) + fullNodeSize);

    enum writeStatus ret = writeNode(fp, node, fullNodeSize - NODE_HEAD_SIZE);
    return ret == WRITE_SUCCESS ? CRUD_SUCCESS : CRUD_FAIL;
}

enum crudStatus insertNodeStr(FILE* fp, char* string, size_t nodeSize, uint64_t* strPos) {
    size_t length = strlen(string);
    size_t count = length / nodeSize + (length % nodeSize ? 1 : 0);
    struct keyNode* tmpNode = malloc(NODE_SIZE);

    char* tmpData = string;
    size_t pos = (size_t)ftell(fp);
    uint64_t fakePos;

    fseek(fp, 0, SEEK_END);
    *strPos = ftell(fp);

    for (size_t i = 0; count > i; i++) {
        if (count - 1 == i)
            tmpNode->header.nextNode = 0;
        else
            tmpNode->header.nextNode = pos + (nodeSize + NODE_HEAD_SIZE) * (i + 1);

        if (0 == i)
            tmpNode->header.prevNode = 0;
        else
            tmpNode->header.prevNode = pos + (nodeSize + NODE_HEAD_SIZE) * (i - 1);

        tmpNode->data = (uint64_t*)(tmpData + nodeSize * i);
        insertNode(fp, tmpNode, nodeSize + NODE_HEAD_SIZE, &fakePos);
    }
    free(tmpNode);
    return CRUD_SUCCESS;
}

size_t appendToIdArray(FILE* fp, uint64_t offset) {
    size_t id;
    struct treeSchema* schema = malloc(SCHEMA_SIZE);
    readTreeSchema(schema, fp);

    uint64_t src = ftell(fp);
    uint64_t realNodeSize = getIdArraySize(schema->meta->templateSize, schema->meta->curId);
    if (!((schema->meta->curId + 1) % realNodeSize)) {
        fseek(fp, 0, SEEK_END);
        uint64_t curEnd = ftell(fp);
        ftruncate(fileno(fp), curEnd + getRealNodeSize(schema->meta->templateSize) + NODE_HEAD_SIZE);

        swapNode(fp, curEnd, src, getRealNodeSize(schema->meta->templateSize));

        freeTreeSchema(schema);
        schema = malloc(SCHEMA_SIZE);
        readTreeSchema(schema, fp);
    }

    schema->root[schema->meta->curId] = offset;
    schema->meta->curId++;
    id = schema->meta->curId - 1;

    fseek(fp, 0, SEEK_SET);
    if (writeTreeSchema(fp, schema) != WRITE_SUCCESS) {
        printf("write error storageBaseApi at 229");
    }
    freeTreeSchema(schema);
    return id;
}

enum crudStatus removeFromIdArray(FILE* fp, uint64_t id, uint64_t* offset) {
    fseek(fp, 0, SEEK_SET);
    struct treeSchema* schema = malloc(SCHEMA_SIZE);
    readTreeSchema(schema, fp);

    if (schema->root[id] == 0) {
        freeTreeSchema(schema);
        return CRUD_FAIL;
    }
    else {
        *offset = schema->root[id];
        if (schema->meta->curId - 1 == id) {
            schema->meta->curId--;
        }
        schema->root[id] = 0;

        writeTreeSchema(fp, schema);
        freeTreeSchema(schema);

        return CRUD_SUCCESS;
    }
}

enum crudStatus convItoO(FILE* fp, uint64_t id, uint64_t* offset) {
    fseek(fp, 0, SEEK_SET);
    struct treeSchema* schema = malloc(SCHEMA_SIZE);
    readTreeSchema(schema, fp);
    if (schema->root[id] == 0) {
        freeTreeSchema(schema);
        return CRUD_FAIL;
    }
    else {
        *offset = schema->root[id];
        freeTreeSchema(schema);
        return CRUD_SUCCESS;
    }

}

enum crudStatus convOtoI(FILE* fp, uint64_t* id, uint64_t offset) {

    struct treeSchema* schema = malloc(SCHEMA_SIZE);
    readTreeSchema(schema, fp);

    struct keyNode* node;
    fseek(fp, offset, SEEK_SET);
    readNode(fp, &node, schema->meta->templateSize, NODE_TYPE_USUAL);

    enum crudStatus ret = CRUD_SUCCESS;

    if (schema->root[node->header.allocNode] == offset)
        *id = node->header.allocNode;
    else
        ret = CRUD_FAIL;

    freeTreeSchema(schema);
    freeNode(node);

    return ret;
}

enum crudStatus changeStrNode(FILE* fp, uint64_t offset, char* newStr, uint64_t size) {
    struct keyNode* curNode = NULL;
    uint64_t lenght = strlen(newStr);
    uint64_t old_offset = offset;
    do {
        offset = old_offset;
        fseek(fp, offset, SEEK_SET);
        readNode(fp, &curNode, size, NODE_TYPE_STRING);

        fseek(fp, offset, SEEK_SET);
        curNode->data = (uint64_t*)(newStr);
        newStr = newStr + size;
        writeNode(fp, curNode, size);

        old_offset = curNode->header.nextNode;
        lenght -= size;
    } while (curNode->header.nextNode && lenght > 0);

    uint64_t fpos;
    if (lenght > 0) {
        insertNodeStr(fp, newStr, size, &fpos);
        curNode->header.nextNode = fpos;
        fseek(fp, offset, SEEK_SET);
        writeNode(fp, curNode, size);
    }
    return CRUD_SUCCESS;
}


