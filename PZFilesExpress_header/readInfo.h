#ifndef READINFO_H
#define READINFO_H
#include "global.h"
extern void sendReadInfo();
static void writeBlkMsgToFile(char *downloadBlocks_p,int totalBlocks_p);
#endif