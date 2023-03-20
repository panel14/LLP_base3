#include "handler.h"

void handleQuery(FILE* fp, Query query, char** _response) {
    char* response = "";
    char tmp[256];
    size_t errorCode;
    size_t templateSize;

    struct treeSchema* schema = malloc(SCHEMA_SIZE);
    readTreeSchema(schema, fp);

    templateSize = schema->meta->templateSize;
    uint32_t* types= malloc(sizeof(uint32_t) * templateSize);
    char** attrNames = malloc(sizeof(char*) * templateSize);

    for (int i = 0; i < templateSize; i++) {
        types[i] = schema->nodesTemplate[i]->header->type;
        attrNames[i] = schema->nodesTemplate[i]->attributeName;
    }

    struct resultSetNode *result = NULL;

    switch (query.command) {
        case 0:
            errorCode = storageFindNodeByFilters(fp, query.filtersList, query.filtersList_count, &result, attrNames);
            if (errorCode == 3) {
                response = concat(response, "At least one of your fields doesn't exist.\n");
                break;
            }
            if (result != NULL) {
                response = concat(response, "<---- Receive Result ---->\n");
                do {
                    sprintf(tmp, "<---- Node %3zu---->\n", result->id);
                    response = concat(response, tmp);

                    for (size_t iter = 0; iter < templateSize; iter++) {
                        if (types[iter] == STRING) {
                            char* s;
                            if (query.filtersList_count == 1 && query.filtersList[0].compList_count == 1 && strcmp(query.filtersList[0].compList[0].kv.key, "id") == 0) {
                                s = (char *) result->value->data[iter];
                            }
                            else {
                                readNode(fp, &s, templateSize, result->value->data[iter], NODE_TYPE_STRING);
                            }
                            sprintf(tmp, "%-20s %s\n", attrNames[iter], s);
                            response = concat(response, tmp);
                            free(s);
                        }
                        else if (types[iter] == INT || types[iter] == BOOL) {
                            sprintf(tmp, "%-20s %lu\n", attrNames[iter], result->value->data[iter]);
                            response = concat(response, tmp);
                        }
                        else if (types[iter] == FLOAT) {
                            double res;
                            memcpy(&res, &(result->value->data[iter]), sizeof(result->value->data[iter]));
                            sprintf(tmp, "%-20s %.6f\n", attrNames[iter], res);
                            response = concat(response, tmp);
                        }
                    }
                    result = result->prev;
                } while (result != NULL);
            }
            else
                response = concat(response,"<---- No Results ---->\n");
            break;
        case 1:
            errorCode = storageFindNodeByFilters(fp, query.filtersList, query.filtersList_count, &result, attrNames);
            if (errorCode == 3) {
                response = concat(response, "At least one of your fields doesn't exist.\n");
                break;
            }
            if (result != NULL) {
                response = concat(response, "<---- Remove result ---->\n");
                do {
                    if (removeNode(fp, result->id, 0) == CRUD_FAIL) {
                        response = concat(response, "Already removed");
                    }
                    else {
                        response = concat(response, "Removed successfully");
                    }
                    sprintf(tmp, "id: %lu\n", (uint64_t) result->id);
                    response = concat(response, tmp);
                    result = result->prev;
                } while (result != NULL)
            }
            else
                response = concat(response,"<---- No Results ---->\n");
            break;
        case 2:
            errorCode = storageInsertNodeNew(fp, query.settingsList, query.settingsList_count, query.filtersList[0].compList[0].kv.valueInt, templateSize, types, attrNames);

            switch (errorCode) {
                case 1:
                    sprintf(tmp, "Field does not match pattern\n");
                    response = concat(response, tmp);
                    break;
                case 2:
                    sprintf(tmp, "Value is not bool\n");
                    response = concat(response, tmp);
                    break;
                case 3:
                    sprintf(tmp, "Value is not float\n");
                    response = concat(response, tmp);
                    break;
                case 4:
                    sprintf(tmp, "Value is not integer\n");
                    response = concat(response, tmp);
                    break;
                case 5:
                    sprintf(tmp, "Value is not string\n");
                    response = concat(response, tmp);
                    break;
                case 6:
                    sprintf(tmp, "Wrong number of parameters\n");
                    response = concat(response, tmp);
                    break;
                default:
                    sprintf(tmp, "Added successfully\n");
                    response = concat(response, tmp);
            }
            break;
        case 3:
            errorCode = storageFindNodeByFilters(fp, query.filtersList, query.filtersList_count, &result, attrNames);
            if (errorCode == 3) {
                response = concat(response, "At least one of your fields doesn't exist.");
                break;
            }
            if (result != NULL) {
                response = concat(response, "<---- Update Result ---->\n");
                do {
                    errorCode = storageUpdateNodeNew(fp, result->id, query.settingsList, query.settingsList_count, templateSize, types, attrNames);

                    if (errorCode != 0) {
                        sprintf(tmp, "Error code: %zu\n", errorCode);
                        response = concat(response, tmp);
                    }

                    sprintf(tmp, "Updated id: %lu\n", (uint64_t) result->id);
                    response = concat(response, tmp);
                    result = result->prev;
                } while (result != NULL);
            }
            else response = concat(response, "<---- No Results ---->\n");
            break;
    }

    *_response = response;
    freeTreeSchema(schema);
    free(types);
    freeTreeSchema(attrNames)
}
