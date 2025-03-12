#ifndef GLOBAL_H
#define GLOBAL_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/timeb.h>
#include <time.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include "zlog.h"
#define true 1
#define false 0
#define SERVER_PORT 10001
#define MAX_SOCKET_BUFFER_SIZE 500*1024
#define SUBFILE_AMOUNT_MAX 1024
#define BLOCKS_AMOUNT_IN_SUBFILE 20
#define PATH_LENGTH 256
#define MESSAGEFILENAME "./message.txt"
#define DOWNLOAD 1
#define UPLOAD 2
#define CarOrUser 1 //人工运行为0，车辆运行为1
#define CARCONFIGFILENAME "./carConfig.ini"


// #define ZLOG_INIT_FAILURE 0x01
// #define AUTODOWNLOAD_FIFO_OPEN_FAILURE 0x02
// #define AUTODOWNLOAD_THREAD_OPEN_FAILURE 0x03
// #define MKYEARDIR_FAILURE 0x04
// #define MKMONTHDIR_FAILURE 0x05


#define FTPSOCKET_OPEN_FAILURE 0x0A
#define DATASOCKET_OPEN_FAILURE 0x1A
#define FTP_CONNECT_FAILURE 0x2A


#define DDU_TO_FDL_START 0x01
#define FDL_TO_DDU_START 0x02
#define FDL_TO_DDU_END   0x03
#define FDL_TO_DDU_ERROR 0x04
#define DO_NOTHING 0x05

typedef struct DDU_TO_FDL
{
    unsigned int command;
    int speed;
}DDU_TO_FDL;

typedef struct FDL_TO_DDU
{
    unsigned int command;
}FDL_TO_DDU;

//FDL_READINFO
typedef enum {
    READINFO = 0x01,
    DOWNLOADREQUEST = 0x02,
    DOWNLOADSTATUS = 0x03,
    CLEAR = 0x04
}EFDCmd;
typedef struct 
{
    char startTag[8];
    EFDCmd cmd;
    char endTag[8];
}TFDFrameHead;
typedef struct{
    char startTag[8];
    int flashID;
    char endTag[8];
}TFDReadInfoFrame;

typedef struct
{
unsigned int amount;
unsigned int current_block;
unsigned int current_chip;
unsigned int current_page;
unsigned int data_waiting;
 /*From here the ID page information starts*/
} __attribute__((packed)) fl04_id_info_r;

typedef struct
{
unsigned int pages_used;
unsigned int pages_read;
unsigned int chip;
unsigned int block;
unsigned int current_counter;
unsigned int erase_counter;
unsigned int first_timestamp;
} __attribute__((packed)) fl04_id_info_page;

typedef struct 
{
    char startTag[8];
    int flashId;
    char *blocks;
    char endTag[8];
}TFDDownloadRequestFrame;

typedef struct 
{
    char startTag[8];
    char path[PATH_LENGTH];
    char endTag[8];
}TFDDownloadRequestReceiveFrame;

typedef enum
{
	SUSPENDING = 0,		// 初始状态
	CREATING = 1,		// flash_downloader设置，文件正在生成
	CREATED = 2,		// flash_downloader设置，文件已生成
	DOWNLOADING = 3,	// 客户端设置，文件正在下载
	DOWNLOADED = 4,	// 客户端设置，文件已被下载
	DONE = 5,			// flash_downloader设置，文件已被删除
	CANCELING = 6,		// 客户端设置，下载被取消
	CANCELED = 7		// flash_downloader设置，下载取消完成
} ESubFileStatus;

typedef struct{
    ESubFileStatus status;
    char path[PATH_LENGTH];
    fl04_id_info_page blocks[BLOCKS_AMOUNT_IN_SUBFILE];
    int blocksAmount;
} TSubFile;

typedef struct 
{
    char startTag[8];
    char *subfiles;
    char endTag[8];
}TFDDownloadStatusFrame;

typedef struct  {
    int ftp_ControlSocket;
    char targetIp[32];
    char username[32];
    char passwd[32];
    char configFilePath[256];
    char localFilePath[256];
    int index;
    int downloadFileAmount;
}ThreadArgs;

typedef struct {
    int opt_h;//help
    int opt_v;//version
    int opt_c;//configure
    int opt_R;//ReadInfo
    int opt_Dr;//DOWNLOADREQUEST请求帧
    int opt_Ds;//DOWNLOADSTATUS请求帧
    int opt_t;//time
    int opt_f;//ftp
}UserCmd;

extern char carIp[32];
extern int flashId;
extern char carName[32];
extern char carPasswd[32];
extern char hostIp[32];
extern char hostName[32];
extern char hostPasswd[32];
extern char fdlSaveDir[128];
extern int hour;
extern int min;
extern int sec;
extern int sleepTime;
extern char start_Time[64];
extern char end_Time[64];



extern char socketBuffer[MAX_SOCKET_BUFFER_SIZE];
extern char totalBlocks[BLOCKS_AMOUNT_IN_SUBFILE*SUBFILE_AMOUNT_MAX*sizeof(fl04_id_info_page)];
extern char downloadTotalBlocks[BLOCKS_AMOUNT_IN_SUBFILE*SUBFILE_AMOUNT_MAX*sizeof(fl04_id_info_page)];
extern int carBlocksAmount;
extern fl04_id_info_page *carBlocks;
extern char totalSubfiles[SUBFILE_AMOUNT_MAX*sizeof(TSubFile)];


extern int readStringFromIni(const char *filename, const char *section, const char *key, char *output, size_t size);
extern int writeStringToIni(const char *filename, const char *section, const char *key, const char *value);
extern void readCarConfigFile(char *carConfigFileName_p);
extern void readConfigFile(char *filename_p,char *targetIp_p,char *flashId_C_p);
extern int offsetFile(const char *filename_p,char *targetStr);
extern void printHelpDoc();
extern void getIpAndId(char *str_p,char *targetIp_p,char *flashId_C_p);
extern int connectServer(char *targetIp_p,int flashId_p,bool *ret_connectServer_p);
extern int gettimeStamp(char *time);

typedef struct  {
    int threadResult;
    //int zlogInitialized;
    int initDownload;
}PingThreadArgs;
extern FDL_TO_DDU fdl_to_ddu;
extern DDU_TO_FDL ddu_to_fdl;
extern int trainSpeed;
extern void progress_Download_Or_Upload(int progress_p,int download_or_upload_p);
extern int isUpTime(int hour_p,int min_p,int sec_p);

#endif // GLOBAL_H
