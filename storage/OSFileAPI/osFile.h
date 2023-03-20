#pragma once
#include <inttypes.h>
#include <stdio.h>

enum openStatus openOrCreateFile(char* filename, FILE** fp, char* mode);
void closeFile(FILE* fp);
int deleteFile(char* filename);
enum readStatus readFile(FILE* fp, void* buffer, size_t size);
enum writeStatus writeFile(FILE* fp, void* buffer, size_t size);

enum readStatus {
	READ_SUCCESS = 0,
	READ_UNEXPECTED_EOF,
	READ_LEN_ERROR,
	READ_FAIL
};

enum writeStatus {
	WRITE_SUCCESS = 0,
	WRITE_LEN_ERROR,
	WRITE_UNEXPECTED_EOF,
	WRITE_FAIL
};

enum openStatus {
	OPEN_SUCCESS = 0,
	OPEN_FAIL
};

enum closeStatus {
	CLOSE_SUCCESS = 0,
	CLOSE_FAIL
};
