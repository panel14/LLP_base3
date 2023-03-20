#include <stdbool.h>

#include "storageFilePublic.h"

int storageOpenOrCreateFile(char* filename, char* mode, FILE** fp) {
	enum openStatus ret = openOrCreateFile(filename, fp, mode);
	if (ret == OPEN_SUCCESS) return 0;
	else {
		printf("File opening failed.");
	}
	return 1;
}

size_t storageFindNodeByFilters(FILE* fp, Query_Filter* filt, size_t filtCount, struct resultSetNode** result, char** templateNames) {
    uint32_t* types;
    size_t size;
    getTypes(fp, &types, &size);

    struct treeSchema* schema = malloc(SCHEMA_SIZE);
    readTreeSchema(schema);

    struct keyNode* curNode = NULL;
    uint64_t fieldNum;
    uint8_t valid;

    if (filtCount == 1 && filt[0].compList_count == 1 && strcmp(filt[0]compList[0].kv.key, "parent") == 0) {
        findByParent(fp, filt[0].compList[0].kv.valueInt, result);
    }
    else if (filtCount == 1 && filt[0].compList_count == 1 && strcmp(filt[0].compList[0].kv.key, "id") == 0) {
        uint64_t* fields;

        enum crudStatus status = getNode(fp, &fields, filt[0].compList[0].kv.valueInt);
        if (status == CRUD_SUCCESS) {
            curNode = malloc(NODE_SIZE);
            memcpy(curNode->data, fields, sizeof(uint64_t) * size);
        }
        appendToResultSet(&curNode, filt[0].compList[0].kv.valueInt, result);
        free(fields);
    }
    else {
        for (size_t i = 0; i < schema->meta->templateSize; i++) {
            if (schema->root[i] == 0) continue;

            fseek(fp, schema->root[i], SEEK_SET);
            readNode(fp, &curNode, size, NODE_TYPE_USUAL);

            for (int fIdx = 0; fIdx < filtCount; fIdx++) {
                valid = filt[fIdx].compList_count;

                for (int compIdx = 0; compIdx < filt[fIdx].compList_count; compIdx++) {
                    char* field = filt[fIdx].compList[compIdx].kv.key;

                    fieldNum = -1;

                    for (int j = 0; j < size; j++) {
                        if (strcmp(templateNames[j], field) == 0) {
                            fieldNum = j;
                        }
                    }

                    if (fieldNum == -1) {
                        return 3;
                    }

                    uint64_t type = types[fieldNum];

                    if (type == STRING) {
                        char* s;
                        readNode(fp, &s, size, curNode->data[fieldNum], NODE_TYPE_STRING);

                        switch (filt[fIdx].compList[compIdx].operation) {
                            case 0:
                                if (strcmp(s, filt[fIdx].compList[compIdx].kv.valueString) != 0) {
                                    valid--;
                                }
                                break;
                            case 5:
                                if (strcmp(s, filt[fIdx].compList[compIdx].kv.valueString) == 0) {
                                    valid--;
                                }
                                break;
                        }

                        free(s);
                    }
                    else {
                        switch (filt[fIdx].compList[compIdx].operation) {
                            case 0:
                                if (curNode->data[fieldNum] != filt[fIdx].compList[compIdx].kv.valueInt) valid--;
                                break;
                            case 1:
                                if (curNode->data[fieldNum] >= filt[fIdx].compList[compIdx].kv.valueInt) valid--;
                                break;
                            case 2:
                                if (curNode->data[fieldNum] > filt[fIdx].compList[compIdx].kv.valueInt) valid--;
                                break;
                            case 3:
                                if (curNode->data[fieldNum] <= filt[fIdx].compList[compIdx].kv.valueInt) valid--;
                                break;
                            case 4:
                                if (curNode->data[fieldNum] < filt[fIdx].compList[compIdx].kv.valueInt) valid--;
                                break;
                            case 5:
                                if (curNode->data[fieldNum] == filt[fIdx].compList[compIdx].kv.valueInt) valid--;
                                break;
                        }
                    }
                }
                if (fIdx == filtCount - 1 && valid) appendToResultSet(&curNode, i, result);
                else if (!valid) break;
            }
        }
    }
    freeTreeSchema(schema);
    free(types);
    return 0;
}

size_t storageInsertNodeNew(FILE* fp, Query_ValueSetting* settings, size_t settingsCount, uint64_t parentId, size_t templateSize, const uint32_t* types, char** attrNames) {
    uint64_t fields[templateSize];
    size_t pPos = -1;

    for (int sIdx = 0; sIdx < settingsCount; sIdx++) {
        for (size_t iter = 0 ; iter < templateSize; iter++) {
            if (strcmp(settings[sIdx].kv.key, attrNames[iter]) == 0) {
                pPos = iter;
                break;
            }
        }
        if (pPos == -1) {
            return PATTERN_ERROR;
        }

        double val;

        switch (types[pPos]) {
            case BOOL:
                if (settings[sIdx].kv.valueInt == 1)
                    fields[pPos] = true;
                else if (settings[sIdx].kv.valueInt == 0)
                    fields[pPos] = false;
                else return BOOL_ERROR;
                break;
            case FLOAT:
                if (settings[sIdx].kv.valueType != 2) return FLOAT_ERROR;
                val = settings[sIdx].kv.valueReal;
                memcpy(&fields[pPos], &val, sizeof(val));
                break;
            case INT:
                if (settings[sIdx].kv.valueType != 1) return INT_ERROR;
                fields[pPos] = settings[sIdx].kv.valueInt;
                break;
            case STRING:
                if (settings[sIdx].kv.valueType != 0) return STR_ERROR;
                fields[pPos] = (uint64_t) settings[sIdx].kv.valueString;
                break;
        }
        pPos = -1;
    }
    if (settingsCount == templateSize)
        addNode(fp, fields, parentId);
    else
        return COUNT_ERROR;
    return OK;
}

size_t storageUpdateNodeNew(FILE* fp, uint64_t id, Query_ValueSetting* settings, size_t settingsCount, size_t templateSize, const uint32_t* types, char** attrNames) {
    int8_t pPos = -1;
    uint64_t value;

    for (int sIdx = 0; sIdx < settingsCount; sIdx++) {
        for (size_t iter = 0; iter < templateSize; iter++) {
            if (strcmp(settings[sIdx].kv.key, attrNames[iter]) == 0) {
                pPos = iter;
                break;
            }
        }

        if (pPos == -1) {
            printf("'%s' field does not match pattern\n", settings->kv.key);
            return 3;
        }

        double val;
        switch (types[pPos]) {
            case BOOL:
                if (settings[sIdx].kv.valueInt == 1)
                    value = true;
                else if(settings[sIdx].kv.valueInt == 0)
                    value = false;
                else {
                    printf("Not bool value : %lu\n", settings[sIdx].kv.valueInt);
                    return 4;
                }
                break;
            case FLOAT:
                if (settings[sIdx].kv.valueType != 2) {
                    printf("Not float value: %f\n", settings[sIdx].kv.valueReal);
                    return 4;
                }
                val = settings[sIdx].kv.valueReal;
                memcpy(&value, &val, sizeof(val));
                break;
            case INT:
                if (settings[sIdx].kv.valueType != 1) {
                    printf("Not int value: %lu\n", settings[sIdx].kv.valueInt);
                    return 4;
                }
                value = settings[sIdx].kv.valueInt;
                break;
            case STRING:
                if (settings[sIdx].kv.valueType != 0) {
                    printf("Not string value: %s\n", settings[sIdx].kv.valueString);
                    return 4;
                }
                char* tmp = malloc(64);
                strcpy(tmp, settings[sIdx].kv.valueString);
                value = (uint64_t) tmp;
                break;
        }

        updateNode(fp, pPos, &value, id);
        pPos = -1;
    }
    return 0;
}

void storageRetriveNode(FILE* fp, char** argv, size_t templateSize, const uint32_t* templateTypes, char** templateNames, size_t count) {
	struct resultSetNode* resultSet = NULL;

	if (strcmp(argv[1], "parent") == 0) {
		if (count == 3) {
			if (isNumeric(argv[2])) {
				findByParent(fp, atoi(argv[1]), &resultSet);
			}
			else {
				printf("Wrong parent id format: %s\n", argv[2]);
			}
		}
		else {
			printf("Wrong number of arguments: %lu\n", count - 1);
		}
	}
	else if (strcmp(argv[1], "field") == 0) {
		if (count == 4) {
			int fIdx = -1;
			for (int i = 0; i < templateSize; i++) {
				if (strcmp(templateNames[i], argv[2]) == 0) {
					fIdx = i;
				}
			}

			if (fIdx == -1) printf("Field does not exist: %s\n", argv[2]);
			else {
				uint32_t type = templateTypes[fIdx];
				switch (type)
				{
				case INT:
					if (isNumeric(argv[3])) {
						uint64_t condition = atoi(argv[3]);
						findByField(fp, fIdx, &condition, &resultSet);
					}
					else printf("Value cannot be interpreted as integer: %s\n", argv[3]);
					break;
				case FLOAT:
					if (strtod(argv[3], NULL) != 0) {
						double tmpCondition = strtod(argv[3], NULL);
						uint64_t condition;
						memcpy(&condition, &tmpCondition, sizeof(tmpCondition));
						findByField(fp, fIdx, &condition, &resultSet);
					}
					else printf("Value cannot be interpreted as float: %s\n", argv[3]);
					break;
				case STRING:
					findByField(fp, fIdx, (uint64_t*)argv[3], &resultSet);
					break;
				case BOOL:
					if (strcmp(argv[3], "True") == 0) {
						bool condition = true;
						findByField(fp, fIdx, &condition, &resultSet);
					}
					else if (strcmp(argv[3], "False") == 0) {
						bool condition = false;
						findByField(fp, fIdx, &condition, &resultSet);
					}
					else printf("Value cannot be interpreted as bool: %s\n", argv[3]);
					break;
				default:
					printf("Unknown type: %s\n", argv[3]);
					break;
				}
			}
		}
		else printf("Wrong number of arguments: %lu", argv[3]);
	}

	else if (strcmp(argv[1], "id") == 0) {
		if (count == 3) {
			if (isNumeric(argv[2])) {
				uint64_t id = atoi(argv[2]);

				struct treeSchema* schema = malloc(SCHEMA_SIZE);
				readTreeSchema(schema, fp);

				uint64_t* fields;

				if (schema->meta->curId < id) {
					printf("Id is too large: %lu\n", id);
					freeTreeSchema(schema);
				}

				enum crudStatus ret = getNode(fp, &fields, id);

				if (ret) {
					printf("No result of operation\n");
					freeTreeSchema(schema);
				}

				for (size_t i = 0; i < schema->meta->templateSize; i++) {
					double fValue;
					char* curName = schema->nodesTemplate[i]->attributeName;
					uint64_t curField = fields[i];
					switch (schema->nodesTemplate[i]->header->type)
					{
					case INT:
						printf("%-20s: %ld\n", curName, curField);
						break;
					case BOOL:
						printf("%-20s: %d\n", curName, curField != 0);
						break;
					case FLOAT:
						memcpy(&fValue, &curField, sizeof(curField));
						printf("%-20s: % lf\n", curName, fValue);
					default:
						printf("%-20s: %s\n", curName, (char *)curField);
						free((char *)fields[i]);
						break;
					}
				}
				free(fields);
				freeTreeSchema(schema);
			}
			else printf("Wrong id format\n");
		}
		else printf("Wrong number of arguments: %lu", count - 1);
	}
	else printf("Incorrect paremeter: aaa\n");

	if (resultSet != NULL) {
		printf("<----- Result set ----->\n");
		do {
			printf("id: %lu\n", (uint64_t) resultSet->id);
			resultSet = resultSet->prev;
		} while (resultSet != NULL);
	}
	else if (strcmp(argv[1], "id") != 0)
		printf("No result present\n");
}

enum storageOp storageInsertNode(FILE* fp, char** str, size_t templateSize, const uint32_t* templateTypes, char** templateNames) {
	char** attrNames;
	size_t count;
	uint64_t fields[templateSize];
	size_t pPos = -1;

	if (!isNumeric(str[1])) {
		printf("Wrong id format: %s", str[1]);
		return ID_FORMAT_FAIL;
	}
	
	for (size_t i = 2; i < templateSize + 2; i++) {
		count = split(str[i], '=', &attrNames);

		if (count != 2)
			return WRONG_ARGS_NUMBER_FAIL;
		for (size_t j = 0; j < templateSize; j++) {
			if (strcmp(attrNames[0], templateNames[j]) == 0) {
				pPos = j;
				break;
			}
		}
		if (pPos == -1) {
			printf("'%s' field does not match template", str[i]);
			return TEMPLATE_FAIL;
		}

		double value;
		switch (templateTypes[pPos])
		{
			case BOOL:
				if (strcmp(attrNames[1], "True") == 0)
					fields[pPos] = true;

				else if (strcmp(attrNames[1], "False") == 0)
					fields[pPos] = false;

				else {
					printf("Value cannot be interpreted as bool: %s", attrNames[1]);
					return CONVERSION_FAIL;
				}
				break;

			case FLOAT:
				value = strtod(attrNames[1], NULL);
				if (value == 0.0) {
					printf("Value cannot be interpreted as float: %s", attrNames[1]);
					return CONVERSION_FAIL;
				}
				memcpy(&fields[pPos], &value, sizeof(value));
				break;

			case INT:
				if (!isNumeric(attrNames[1])) {
					printf("Value cannot be interpreted as integer: %s", attrNames[1]);
					return CONVERSION_FAIL;
				}
				fields[pPos] = strtol(attrNames[1], NULL, 10);
				break;

			case STRING:
				fields[pPos] = (uint64_t)attrNames[1];
				break;

			default:
				break;
		}
		pPos = -1;
		free(attrNames);
	}

	addNode(fp, fields, strtol(str[1], NULL, 10));

	return SUCCESS;
}

enum storageOp storageUpdateNode(FILE* fp, char** str, size_t templateSize, const uint32_t* templateTypes, char** templateNames, size_t fieldsCount) {
	char** attrNames;
	size_t count;
	uint64_t value;
	size_t pPos = -1;

	if (!isNumeric(str[1])) {
		printf("Wrong id format");
		return ID_FORMAT_FAIL;
	}
	for (size_t i = 2; i < fieldsCount; i++) {
		count = split(str[i], '=', &attrNames);
		if (count != 2) {
			printf("Invalid input.\n");
			return WRONG_ARGS_NUMBER_FAIL;
		}
		for (size_t j = 0; j < templateSize; j++) {
			if (strcmp(attrNames[0], templateNames[j]) == 0) {
				pPos = j;
				break;
			}
		}
		if (pPos == -1) {
			printf("'%s' field does not match pattern.\n", str[i]);
			return TEMPLATE_FAIL;
		}

		double val;

		printf("Updated type: %lu\n", templateTypes[pPos]);

		switch (templateTypes[pPos]) {
		case BOOL:
			if (strcmp(attrNames[1], "True") == 0)
				value = true;
			else if (strcmp(attrNames[1], "False") == 0)
				value = false;
			else {
				printf("Value cannot be interpreted as bool: %s", attrNames[1]);
				return CONVERSION_FAIL;
			}
			break;
		case FLOAT:
			val = strtod(attrNames[1], NULL);
			if (val == 0.0) {
				printf("Value cannot be interpreted as float: %s", attrNames[1]);
				return CONVERSION_FAIL;
			}
			memcpy(&value, &val, sizeof(val));

			break;
		case INT:
			if (!isNumeric(attrNames[1])) {
				printf("Value cannot be interpreted as integer: %s", attrNames[1]);
				return CONVERSION_FAIL;
			}
			value = atoi(attrNames[1]);
			break;
		case STRING:
			value = attrNames[1];
			break;
		}
		updateNode(fp, pPos, &value, atoi(str[1]));
		free(attrNames);
		pPos = -1;
	}

	return SUCCESS;
}

enum storageOp storageDeleteNode(FILE* fp, char* idStr, uint8_t isStr) {
	if (isNumeric(idStr)) {
		if (removeNode(fp, atoi(idStr), isStr) == CRUD_SUCCESS) return SUCCESS;
		else return ID_NOT_EXIST_FAIL;
	}
	return ID_FORMAT_FAIL;
}

void storageCloseFile(FILE* fp) {
	closeFile(fp);
}

void storageGetHelp() {
	printf("\"add\" --> Add node to tree. Command format:\n");
	printf("add <parent_id> <key1>=<value1> <key2>=<value2> ...\n--------\n");

	printf("\"update\" --> Update one or more node's fields. Command format\n");
	printf("update <node_id> <key1>=<upd_value1> <key2>=<upd_value2> ...\n--------\n");

	printf("\"remove\" --> Remove node recursively (with all node's children). Command format:\n");
	printf("remove <node_id>\n--------\n");

	printf("\"find_by field\" --> Find node (one or more) by specified field. Command format:\n");
	printf("find_by field <field name> <value>\n--------\n");

	printf("\"find_by id\" --> Find node by id. Command format:\n");
	printf("find_by id <id>\n--------\n");

	printf("\"find_by parent\" --> Find node by parent. Command format:\n");
	printf("find_by parent <parent id>\n--------\n");

	printf("\"print schema\" --> Print tree schema. Command format:\n");
	printf("print schema\n--------\n");

	printf("\"print nodes\" --> Print nodes array. Command format:\n");
	printf("print nodes\n--------\n");

	printf("\"exit\" --> Exit the program. Command format:\n");
	printf("exit\n--------\n");
}
