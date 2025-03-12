#ifndef DOWNLOAD_H
#define DOWNLOAD_H
#include "global.h"
#include "ftp_client.h"


static char username[20];
static char passwd[20];
//time
extern void getTime_s_e(char *time);


//downloadRequest
extern bool sendDownloadRequest(char *fdlFileName_p);
static int readBlockMessage(char *downloadTotalBlocks_p,char *start_Time_p,char *end_Time_p);
static bool writeLogFile(char *targetIp_p,char *configFilePath_p,const char *localFilePath_p);
static void getSubFileStatus(int status,int index);
static void writeSubfileMessageToFile(char *Subfiles_p,int totalSubfiles_p,bool *ret_writeSubfileMessageToFile_p);
extern void writeMesssageToConfigFile(char *srcFile,char *srcStr,char *tarStr);

//downloadStatus
extern bool sendDownloadStatus(char *fdlFileName_p);
static int readSubfileMessage(char *downloadTotalSubfiles_p);
static void deleteSubfileLog(char *fdlFileName_p);
static void deleteSubfile();
//static void deleteSubfileMessage();
static void downloadFromServerSubfile(ThreadArgs *args_p,char *fdlFileName_p);
#endif
