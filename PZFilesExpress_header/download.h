#ifndef DOWNLOAD_H
#define DOWNLOAD_H
#include "global.h"
#include "ftp_client.h"

typedef struct  {
    char configFilePath[256];
    char localFilePath[256];
    int index;
    int downloadFileAmount;
}DownloadStatusArgs;

static char username[20];
static char passwd[20];
//time
extern void getTime_s_e(char *time);


//downloadRequest
extern void sendDownloadRequest();
static int readBlockMessage(char *downloadTotalBlocks_p,char *start_Time_p,char *end_Time_p);
static void writeLogFile(char *configFilePath_p,const char *localFilePath_p);
static void getSubFileStatus(int status,int index);
static void writeSubfileMessageToFile(char *Subfiles_p,int totalSubfiles_p);

//downloadStatus
extern void sendDownloadStatus();
static int readSubfileMessage(char *downloadTotalSubfiles_p);
static void deleteSubfile();
static int downloadFromServerSubfile(DownloadStatusArgs *args_p,char *fdlFileName_p);
#endif
