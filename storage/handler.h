#pragma once

#include "message.pb.h"
#include "StorageFilePublicAPI/storageFilePublic.h"

void handleQuery(FILE* fp, Query query, char** _response);