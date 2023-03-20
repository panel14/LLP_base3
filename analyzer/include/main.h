#ifndef LLP_BASE3_MAIN_H
#define LLP_BASE3_MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "include/query.h"
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include "../pblib/message.pb.h"
#include <pb_encode.h>
#include <pb_decode.h>

void setCurOper(uint8_t operation);
void setCurVal(char* key, uint64_t valInt, double valReal);
void addValSetting(char* key, uint64_t valInt, double valReal);
void switchFilter();
void setComp();
void setCommand(uint8_t command);
void *dmalloc(size_t sizeOf);
void printMemory();
void sendData();

#endif
