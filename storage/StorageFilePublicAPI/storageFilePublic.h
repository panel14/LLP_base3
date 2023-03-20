#pragma once
#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include "../StorageFileInnerAPI/storageFileInner.h"
#include "../Utils/sstring.h"
#include "message.pb.h"

enum storageOp {
	SUCCESS = 0,
	ID_FORMAT_FAIL,
	WRONG_ARGS_NUMBER_FAIL,
	TEMPLATE_FAIL,
	CONVERSION_FAIL,
	ID_NOT_EXIST_FAIL,
	FIELD_DOES_NOT_EXIST
};

size_t storageFindNodeByFilters(FILE* fp, Query_Filter* filt, size_t filtCount, struct resultSetNode** result, char** templateNames);
size_t storageInsertNodeNew(FILE* fp, Query_ValueSetting* settings, size_t settingsCount, uint64_t parentId, size_t templateSize, const uint32_t* types, char** attrNames);
size_t storageUpdateNodeNew(FILE* fp, uint64_t id, Query_ValueSetting* settings, size_t settingsCount, size_t templateSize, const uint32_t* types, char** attrNames);

int storageOpenOrCreateFile(char* filename, char* mode, FILE** fp);
void storageRetriveNode(FILE* fp, char** argv, size_t templateSize, const uint32_t* templateTypes, char** templateNames, size_t count);
enum storageOp storageInsertNode(FILE* fp, char** str, size_t templateSize, const uint32_t* templateTypes, char** templateNames);
enum storageOp storageUpdateNode(FILE* fp, char** str, size_t templateSize, const uint32_t* templateTypes, char** templateNames, size_t fieldsCount);
enum storageOp storageDeleteNode(FILE* fp, char* idStr, uint8_t isStr);
void storageCloseFile(FILE* fp);
void storageGetHelp();

enum errCodes {
    OK,
    PATTERN_ERROR,
    BOOL_ERROR,
    FLOAT_ERROR,
    INT_ERROR,
    STR_ERROR,
    COUNT_ERROR
};