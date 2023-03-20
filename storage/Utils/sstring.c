#include "sstring.h"

size_t split(char* str, const char c, char*** arr) {
	int count = 1;
	int i = 0;
	char* p;

	p = str;

	while (*p != '\n' && *p != '\0') {
		if (*p == c) count++;
		p++;
	}

	*arr = (char**)malloc(sizeof(char*) * count);
	if (*arr == NULL) exit(1);

	for (char* pch = strtok(str, &c); pch != NULL; pch = strtok(NULL, &c)) {
		if (i == count - 1 && pch[strlen(pch) - 1] == '\n')
			pch[strlen(pch) - 1] = '\0';
		(*arr)[i++] = pch;
	}

	return count;
}

bool isNumeric(const char* str) {
	while (*str != '\0' && *str != 13) {
		if (*str < '0' || *str > '9') 
			return false;
		str++;
	}
	return true;
}

char* concat(const char* str1, const char* str2) {
	char* res = malloc(strlen(str1) + strlen(str2) + 1);
	strcpy(res, str1);
	strcat(res, str2);
	return res;
}