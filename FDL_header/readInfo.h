#ifndef READINFO_H
#define READINFO_H
#include "global.h"
extern bool sendReadInfo();
static void writeBlockMessageToFile(char *downloadBlocks_p,int totalBlocks_p,bool *ret_writeBlockMessageToFile_p);
#endif