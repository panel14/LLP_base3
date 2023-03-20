#include <time.h>
#include "storageFileInner.h"

#define PRINTED_LEN 16

void appendToResultSet(struct keyNode** addedNodes, uint64_t id, struct resultSetNode** resultSet) {
    if ((*resultSet) == NULL) {
        *resultSet = malloc(RESULT_SET_SIZE);
        (*resultSet)->prev = NULL;
    }
    else {
        struct resultSetNode* newSet = malloc(RESULT_SET_SIZE);
        newSet->prev = *resultSet;
        *resultSet = newSet;
    }
    (*resultSet)->value = *addedNodes;
    (*resultSet)->id = id;
    *addedNodes = malloc(NODE_SIZE);
}

size_t addNode(FILE* fp, uint64_t* values, uint64_t parent) {
	uint32_t* types;
    size_t size;
    getTypes(fp, &types, &size);

    struct treeSchema* schema = malloc(SCHEMA_SIZE);
    readTreeSchema(schema, fp);

    struct keyNode* newNode = malloc(NODE_SIZE);
    union nodeHeader newHeader = { .parentNode = parent, .allocNode = schema->meta->curId };

    newNode->header = newHeader;
    newNode->data = malloc(getRealNodeSize(size));
    uint64_t link;

    for (size_t i = 0; i < size; i++) {
        if (types[i] == STRING) {
            //            char* param = NULL;
            //            memcpy(&param, &fields[iter], sizeof(fields[iter]));
            insertNodeStr(fp, values[i], getRealNodeSize(size), &link);
            newNode->data[i] = link;
        }
        else {
            newNode->data[i] = (uint64_t)values[i];
        }
    }

    size_t fullTupleSize = NODE_HEAD_SIZE + getRealNodeSize(size);

    enum crudStatus status = insertNode(fp, newNode, fullTupleSize, &link);

    linkSton(fp, newNode, link);

    size_t id = appendToIdArray(fp, link);

    freeNode(newNode);
    free(types);
    freeTreeSchema(schema);

    return status;
}

enum crudStatus getNode(FILE* fp, uint64_t** values, uint64_t id) {
    uint64_t offset;
    if (convItoO(fp, id, &offset) == CRUD_FAIL) return CRUD_FAIL;
    struct keyNode* node;
    uint32_t* types;
    size_t size;
    getTypes(fp, &types, &size);

    fseek(fp, offset, SEEK_SET);
    readNode(fp, &node, (uint64_t)size, NODE_TYPE_USUAL);
    *values = malloc(sizeof(uint64_t) * size);

    for (size_t i = 0; i < size; i++) {
        if (types[i] == STRING) {
            char* s;
            readStringFromNode(fp, &s, size, node->data[i]);
            memcpy(&(*values)[i], &s, sizeof(s));
        }
        else {
            (*values)[i] = node->data[i];
        }

    }
    freeNode(node);
    free(types);
    return CRUD_SUCCESS;
}

enum crudStatus removeNode(FILE* fp, uint64_t id, uint8_t strFlag) {
    uint32_t* types;
    size_t size;
    getTypes(fp, &types, &size);
    //    printf("%lu\n", id);

    if (!strFlag) {
        uint64_t offset;
        if (removeFromIdArray(fp, id, &offset) == CRUD_FAIL)
            return CRUD_FAIL;

        for (size_t field_num = 0; field_num < size; field_num++) {
            if (types[field_num] == STRING) {
                struct keyNode* node;
                fseek(fp, (long)offset, SEEK_SET);
                readNode(fp, &node, size, NODE_TYPE_USUAL);

                removeNode(fp, node->data[field_num], 1);
                freeNode(node);
            }
        }

        swapLastNode(fp, offset, getRealNodeSize(size));

        struct resultSetNode* children = NULL;
        findByParent(fp, id, &children);
        while (children != NULL) {
            removeNode(fp, children->id, 0);
            children = children->prev;
        }
    }
    else {
        struct keyNode* strNode;
        while (id != 0) {
            fseek(fp, id, SEEK_SET);
            readNode(fp, &strNode, size, NODE_TYPE_STRING);
            swapLastNode(fp, id, getRealNodeSize(size) + NODE_HEAD_SIZE);

            id = strNode->header.nextNode;
            freeNode(strNode);
        }
    }
    free(types);
    return CRUD_SUCCESS;
}

enum crudStatus findByParent(FILE* fp, uint64_t parent, struct resultSetNode** resultSet) {
    struct treeSchema* schema = malloc(SCHEMA_SIZE);
    readTreeSchema(schema, fp);
    struct keyNode* curNode = malloc(NODE_SIZE);

    for (size_t i = 0; i < schema->meta->curId; i++) {
        if (schema->root[i] == 0) continue;

        fseek(fp, schema->root[i], SEEK_SET);

        readNode(fp, &curNode, schema->meta->templateSize, NODE_TYPE_USUAL);
        if (curNode->header.parentNode == parent) {
            appendToResultSet(&curNode, i, resultSet);
        }
    }
    freeTreeSchema(schema);
    return 0;
}

enum crudStatus findByField(FILE* fp, uint64_t fieldNumber, uint64_t* condition, struct resultSetNode** resultSet) {
    uint32_t* types;
    size_t size;
    getTypes(fp, &types, &size);

    uint64_t type = types[fieldNumber];
    struct treeSchema* schema = malloc(SCHEMA_SIZE);
    readTreeSchema(schema, fp);

    struct keyNode* cur_tuple = NULL;
    for (size_t i = 0; i < schema->meta->curId; i++) {
        if (schema->root[i] == 0) continue;

        fseek(fp, schema->root[i], SEEK_SET);
        readNode(fp, &cur_tuple, size, NODE_TYPE_USUAL);
        if (type == STRING) {
            char* s;
            readStringFromNode(fp, &s, size, cur_tuple->data[fieldNumber]);
            if (!strcmp(s, (char*)condition)) {
                appendToResultSet(&cur_tuple, i, resultSet);
            }
            free(s);
        }
        else {
            if (cur_tuple->data[fieldNumber] == *condition) {
                appendToResultSet(&cur_tuple, i, resultSet);
            }
        }
    }
    freeTreeSchema(schema);
    free(types);
    return 0;
}

enum crudStatus updateNode(FILE* file, uint64_t fieldNumber, uint64_t* newValue, uint64_t id) {
    uint32_t* types;
    size_t size;
    getTypes(file, &types, &size);

    uint64_t type = types[fieldNumber];
    uint64_t offset;
    convItoO(file, id, &offset);

    struct keyNode* curNode;
    fseek(file, offset, SEEK_SET);
    readNode(file, &curNode, size, NODE_TYPE_USUAL);
    if (type == STRING) {
        changeStrNode(file, curNode->data[fieldNumber], (char*)newValue, getRealNodeSize(size));
    }
    else {

        memcpy(&(curNode->data[fieldNumber]), newValue, sizeof(*newValue));
        //        cur_tuple->data[field_number] = *new_value;
        fseek(file, offset, SEEK_SET);

        writeNode(file, curNode, getRealNodeSize(size));

/*        uint64_t invId;
        convOtoI(file, &invId, offset);
        printf("New node id: %lu", invId);*/
    }
    freeNode(curNode);
    free(types);
    return 0;
}

void printNodesFrom(FILE* fp) {
    struct treeSchema schema;
    readTreeSchema(&schema, fp);

    uint32_t* types;
    size_t size;
    getTypes(fp, &types, &size);

    struct keyNode* curNode;

    for (size_t i = 0; i < schema.meta->curId; i++) {
        if (schema.root[i] == 0) continue;

        fseek(fp, schema.root[i], SEEK_SET);
        readNode(fp, &curNode, size, NODE_TYPE_USUAL);
        printf("<----- Node %3zu ----->\n", i);

        for (size_t j = 0; j < size; j++) {
            if (types[j] == STRING) {
                char* s;
                readStringFromNode(fp, &s, schema.meta->templateSize, curNode->data[j]);
                printf("Attribute: %-20s; Value: %s;\n", schema.nodesTemplate[j]->attributeName, s);
                free(s);
            }
            else if (types[j] == INT || types[j] == BOOL) {
                printf("Attribute: %-20s; Value: %lu;\n", schema.nodesTemplate[j]->attributeName, curNode->data[j]);
            }
            else if (types[j] == FLOAT) {
                double res;
                memcpy(&res, &(curNode->data[j]), sizeof(curNode->data[j]));
                printf("Attribute: %-20s; Value: %.6f;\n", schema.nodesTemplate[j]->attributeName, res);
            }
        }
        freeNode(curNode);
    }
    free(types);
}

void printTreeSchema(FILE* fp) {
    struct treeSchema* schema = malloc(SCHEMA_SIZE);
    readTreeSchema(schema, fp);

    printf("<----- Tree schema ----->\n");
    printf("%-20s%ld\n", "Current ID: ", schema->meta->curId);
    printf("%-20s%ld\n", "Template size: ", schema->meta->templateSize);

    printf("<----- Template ----->\n");

    for (size_t i = 0; i < schema->meta->templateSize; i++) {
        printf("Key %3d [Type %3d]: %s\n", schema->nodesTemplate[i]->header->size,
                                           schema->nodesTemplate[i]->header->type,
                                           schema->nodesTemplate[i]->attributeName);
    }

    size_t realIdArraySize = getIdArraySize(schema->meta->templateSize, schema->meta->curId);
    for (size_t i = 0; i < (schema->meta->templateSize / PRINTED_LEN); i++) {
        for (size_t j = 0; j < PRINTED_LEN; j++) {
            printf("%16lx", schema->root[i * PRINTED_LEN + j]);
        }
        printf("\n");
    }
    freeTreeSchema(schema);
}

