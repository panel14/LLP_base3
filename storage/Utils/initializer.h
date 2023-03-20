#pragma once

#include <stdlib.h>
#include "../DataFileAPI/dataFile.h"

void createEmptyTreeSchema(char** template, uint32_t* types, size_t templateSize, size_t* attrSizes, struct treeSchema* schema);