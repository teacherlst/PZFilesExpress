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
#include "cJSON.h"
#include "error.h"
#define SERVER_PORT 10001
#define MAX_SOCKET_BUFFER_SIZE 500*1024
#define SUBFILE_AMOUNT_MAX 1024
#define BLOCKS_AMOUNT_IN_SUBFILE 20
#define PATH_LENGTH 256
#define MESSAGEFILENAME "./message.txt"
#define DOWNLOAD 1
#define UPLOAD 2
#define CFGFILENAME "./carConfig.cfg"
//UDP
#define LOCAL_PORT 10313    // 本地接收端口
#define REMOTE_PORT 10312   // 目标发送端口
#define SEND_BUFFER_SIZE 9
#define RECV_BUFFER_SIZE 31

typedef struct 
{
    char vcuIp[64];
    int flashId;
    char vcuName[32];
    char vcuPasswd[32];
    char remoteHostIp[64];
    char remoteHostName[32];
    char remoteHostPasswd[32];
    char fdlSaveDir[128];
    int hour;
    int min;
    int sec;
    int sleepTime;
    char lastTime[128];
    int isInitDownload;
}PZFilesExpressModel;

typedef struct 
{
    char year[32];
    char month[32];
    char day[32];
}SystemTime;
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

extern PZFilesExpressModel *model;
extern int read_json_to_model(const char *filename, PZFilesExpressModel *model);
extern int write_model_to_json(const char *filename, const PZFilesExpressModel *model);
extern void printModel(const PZFilesExpressModel* model);

extern int sockfd;
extern char fdlFileName[1024];
extern SystemTime systemTime;
extern TFDDownloadRequestFrame downloadRequestFrame;
extern TFDDownloadStatusFrame downloadStatusFrame;
extern TFDDownloadStatusFrame downloadStatusReceiveFrame;
extern char socketBuffer[MAX_SOCKET_BUFFER_SIZE];
extern char totalBlocks[BLOCKS_AMOUNT_IN_SUBFILE*SUBFILE_AMOUNT_MAX*sizeof(fl04_id_info_page)];
extern char downloadTotalBlocks[BLOCKS_AMOUNT_IN_SUBFILE*SUBFILE_AMOUNT_MAX*sizeof(fl04_id_info_page)];
extern int carBlocksAmount;
extern fl04_id_info_page *carBlocks;
extern char totalSubfiles[SUBFILE_AMOUNT_MAX*sizeof(TSubFile)];
extern int offsetFile(const char *filename_p,char *targetStr);
extern int connectServer(char *carIp_p,int flashId_p);
extern int gettimeStamp(char *time);

typedef struct  {
    bool thread_started;
    bool threadResult;
}DownloadThreadArgs;
extern char start_Time[64];
extern char end_Time[64];
extern int trainSpeed;
extern void progress_Download_Or_Upload(int progress_p,int download_or_upload_p);
extern bool isUpTime(int hour_p,int min_p,int sec_p);
extern long getLocalFileSize(const char *fileName_p);

#endif // 
