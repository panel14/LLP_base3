#pragma once

#include <stdio.h>
#include "../../DataFileAPI/dataFile.h"
#include "../../StorageFilePublicAPI/storageFilePublic.h"
#include "../../Utils/parser.h"

#define INPUT_SIZE 1024

void start(FILE* fp);
void initFile(FILE* fp);
FILE* initialize(int argc, char** argv);