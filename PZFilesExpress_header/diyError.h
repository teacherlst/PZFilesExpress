#ifndef DIYERROR_H
#define DIYERROR_H
#include "zlog.h"
#include "setjmp.h"

#define SUCCESS 0

//connectserver
#define SOCKET_ERR -100
#define CONNECT_ERR -101

#define OPEN_FILE_ERR -201
#define WRITE_FILE_ERR -202
#define SEEK_FILE_ERR -203
#define ACCESS_FILE_ERR -204
#define REMOVE_FILE_ERR -205

#define OPEN_DIR_ERR -301
//readInfo
#define RI_CONNECT_SERVER_ERR 0x1F01
#define RI_SEND_ERR 0x1F02
#define RI_FRAME_HEAD_ERR 0x1F03
#define RI_DATA_HEAD_ERR 0x1F04
#define RI_NODATA_ERR 0x1F05
#define RI_DATA_TAIL_ERR 0x1F06

//downloadRequest
#define DR_CONNECT_SERVER_ERR 0x2F01
#define DR_SEND_ERR 0x2F02
#define DR_FRAME_HEAD_ERR 0x2F03
#define DR_DATA_HEAD_ERR 0x2F04
#define DR_NODATA_ERR 0x2F05
#define DR_DATA_TAIL_ERROR 0x2F06
#define DR_NO_MESSAGEFILE_ERR 0x2F07
#define DR_DOWNLOAD_FILE_EMPTY_ERR 0x2F08

//downloadStatus
#define DS_CONNECT_SERVER_ERR 0x3F01
#define DS_SEND_ERR 0x3F02
#define DS_FRAME_HEAD_ERR 0x3F03
#define DS_DATA_HEAD_ERR 0x3F04
#define DS_FTPSOCKET_OPEN_ERR 0x3F05
#define DS_DATA_ERR 0x3F06
#define DS_DATA_TAIL_ERROR 0x3F07
#define DS_SUBFILE_COUNT_ERR 0x3F08
#define DS_NO_MESSAGEFILE_ERR 0x3F09
#define DS_DOWNLOAD_FILE_EMPTY_ERR 0x3F0A
#define DS_TIMEOUT_ERR 0x3F0B
//ftp
#define FTP_SOCKET_ERR 0xFF01
#define FTP_CONNECT_ERR 0xFF02

#define FTP_LOGIN_ERR 0xFF11
#define FTP_MKDIR_ERR 0xFF21
#define FTP_RMDIR_ERR 0xFF31
#define FTP_TYPE_ERROR 0xFF41
#define FTP_SIZE_ERR 0xFF51
#define FTP_PASV_ERROR 0xFF61
#define FTP_DATA_SOCKET_OPEN_ERR 0xFF71
#define FTP_D_DOWNLOAD_ERR 0xFF81
#define FTP_DOWNLOAD_ERR 0xFF91
#define FTP_D_UPLOAD_ERR 0xFFA1
#define FTP_UPLOAD_ERR 0xFFB1
#define GET_LOCAL_FILE_SIZE_ERR 0xFFC1
#define DOWNLOAD_RECV_ERROR 0xFFD1
#define UPLOAD_SEND_ERROR 0xFFE1

extern jmp_buf jump_buffer;
extern char err_name[128];
extern void fopenFileErr(const char *fileName);
extern void fwriteFileErr(const char *fileName);
extern void fseekFileErr(const char *fileName);
extern void fopenDirErr(const char *dirName);
extern void removeFileErr(const char *fileName);
extern void diyError(int code) ;

#endif