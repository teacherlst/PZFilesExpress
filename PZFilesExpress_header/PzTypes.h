#ifndef PZTYPES_H
#define PZTYPES_H

#include <pthread.h>
#ifdef __WIN32
#include <windows.h>
#endif

#define PZFILEDATASOURCE_PORT   10312
#define PZFILESEXPRESS_PORT 10313

typedef unsigned char  pz_bool_t;
typedef unsigned char  pz_uint8_t;
typedef signed char    pz_int8_t;
typedef unsigned short pz_uint16_t;
typedef signed short   pz_int16_t;
typedef unsigned int   pz_uint32_t;
typedef signed int     pz_int32_t;

typedef struct
{
   pz_uint8_t start_char;
   pz_uint8_t fuc_code;
   pz_uint8_t fdl_task_progress;
   pz_uint8_t evr_task_progress;
   pz_uint8_t train_num;
   pz_uint8_t reserved;
   pz_int16_t crc_res;
}ExToDsPack;//PzFilesExpress To DataSource

typedef struct
{
   pz_uint8_t  start_char;
   pz_uint8_t  fuc_code;
   pz_uint16_t pack_num;//reserved
   pz_uint16_t pack_length;
   pz_uint8_t  cab1_stat;
   pz_uint8_t  cab6_stat;
   pz_int16_t  speed;
   pz_uint8_t  manual_cmd;
   pz_uint8_t  train_num;
   pz_uint8_t  reserved[16];
   pz_int16_t  crc_res;
}DsToExPack;//DataSource To PzFilesExpress

typedef void (*handleSpeed)(pz_uint16_t speed);
typedef void (*handleManCmd)(pz_uint8_t cmd);

typedef struct
{
    pz_uint32_t         sockfd;
    DsToExPack          *pack;
#ifdef __linux__
    pthread_t           pid;
    pthread_mutex_t     mutex;
#endif
#ifdef __WIN32
    HANDLE              hThread;
    HANDLE              mutex;
#endif
}DsToExTask;

typedef struct
{
    pz_uint32_t         sockfd;
    ExToDsPack          *pack;
#ifdef __linux__
    pthread_t           pid;
    pthread_mutex_t     mutex;
#endif
#ifdef __WIN32
    HANDLE              hThread;
    HANDLE              mutex;
#endif
}ExToDsTask;

typedef struct PzFilesSeverTask
{
    DsToExTask          *send_task;
    ExToDsTask          *recv_task;
    pz_bool_t           task_finised;
    pz_bool_t           task_initOk;//1=ok else=nok
    char                remote_ip[20];
}PPSTask;

typedef struct PzFilesClientTask
{
    DsToExTask          *recv_task;
    ExToDsTask          *send_task;
    handleSpeed         pSpeedCallBack;
    handleManCmd        pManCmdCallBack;
    pz_bool_t           task_initOk;//1=ok else=nok
    char                remote_ip[20];
}PPCTask;


#endif // PZTYPES_H
