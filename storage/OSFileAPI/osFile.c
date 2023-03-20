#include "osFile.h"

enum openStatus openOrCreateFile(char* filename, FILE** fp, char* mode) {
	*fp = fopen(filename, mode);
	return (NULL == *fp) ? OPEN_FAIL : OPEN_SUCCESS;
}

void closeFile(FILE *fp) {
	fclose(fp);
}

enum readStatus readFile(FILE* fp, void* buffer, size_t size) {
	size_t len = fread(buffer, size, 1, fp);

	if (len < 1) return READ_LEN_ERROR;
	else if (sizeof(buffer) < len) return READ_UNEXPECTED_EOF;

	return READ_SUCCESS;
}

enum writeStatus writeFile(FILE* fp, void* buffer, size_t size) {
	size_t len = fwrite(buffer, size, 1, fp);

	if (len < 1) return WRITE_LEN_ERROR;
	else if (sizeof(buffer) < len) return WRITE_UNEXPECTED_EOF;

	return WRITE_SUCCESS;
}

int deleteFile(char* filename) {
	return remove(filename);
}