#pragma once

#include "../StorageFilePublicAPI/storageFilePublic.h"

size_t split(char* str, const char chr, char*** arr);
bool isNumeric(const char* str);
char* concat(const char* str1, const char* str2);