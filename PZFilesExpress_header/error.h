#ifndef ERROR_H
#define ERROR_H
#include "zlog.h"
#include "setjmp.h"
static jmp_buf jump_buffer = {0};

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
#define RI_CONNECT_SERVER_ERR 0x0001FF01
#define RI_SEND_ERR 0x0001FF02
#define RI_FRAME_HEAD_ERR 0x0001FF03
#define RI_DATA_HEAD_ERR 0x0001FF04
#define RI_NODATA_ERR 0x0001FF05
#define RI_DATA_TAIL_ERR 0x0001FF06

//downloadRequest
#define DR_CONNECT_SERVER_ERR 0x0002FF01
#define DR_SEND_ERR 0x0002FF02
#define DR_FRAME_HEAD_ERR 0x0002FF03
#define DR_DATA_HEAD_ERR 0x0002FF04
#define DR_NODATA_ERR 0x0002FF05
#define DR_DATA_TAIL_ERROR 0x0002FF06
#define DR_NO_MESSAGEFILE_ERR 0x0002FF07
#define DR_DOWNLOAD_FILE_EMPTY_ERR 0x0002FF08

//downloadStatus
#define DS_CONNECT_SERVER_ERR 0x0003FF01
#define DS_SEND_ERR 0x0003FF02
#define DS_FRAME_HEAD_ERR 0x0003FF03
#define DS_DATA_HEAD_ERR 0x0003FF04
#define DS_FTPSOCKET_OPEN_ERR 0x0003FF05
#define DS_DATA_ERR 0x0003FF06
#define DS_DATA_TAIL_ERROR 0x0003FF07
#define DS_SUBFILE_COUNT_ERR 0x0003FF08
#define DS_NO_MESSAGEFILE_ERR 0x0003FF09
#define DS_DOWNLOAD_FILE_EMPTY_ERR 0x0003FF0A

//ftp
#define FTP_SOCKET_ERR 0xFFFFFF01
#define FTP_CONNECT_ERR 0xFFFFFF02

#define FTP_LOGIN_ERR 0xFFFFFF11
#define FTP_MKDIR_ERR 0xFFFFFF21
#define FTP_RMDIR_ERR 0xFFFFFF31
#define FTP_TYPE_ERROR 0xFFFFFF41
#define FTP_SIZE_ERR 0xFFFFFF51
#define FTP_PASV_ERROR 0xFFFFFF61
#define FTP_DATA_SOCKET_OPEN_ERR 0xFFFFFF71
#define FTP_D_DOWNLOAD_ERR 0xFFFFFF81
#define FTP_DOWNLOAD_ERR 0xFFFFFF91
#define FTP_D_UPLOAD_ERR 0xFFFFFFA1
#define FTP_UPLOAD_ERR 0xFFFFFFB1
#define GET_LOCAL_FILE_SIZE_ERR 0xFFFFFFC1
#define DOWNLOAD_RECV_ERROR 0xFFFFFFD1
#define UPLOAD_SEND_ERROR 0xFFFFFFE1

static char err_name[128];
static void fopenFileErr(const char *fileName){
    strcpy(err_name, fileName);
    longjmp(jump_buffer, OPEN_FILE_ERR);
}
static void fwriteFileErr(const char *fileName){
    strcpy(err_name, fileName);
    longjmp(jump_buffer, WRITE_FILE_ERR);
}
static void fseekFileErr(const char *fileName){
    strcpy(err_name, fileName);
    longjmp(jump_buffer, SEEK_FILE_ERR);
}
static void fopenDirErr(const char *dirName){
    strcpy(err_name, dirName);
    longjmp(jump_buffer, OPEN_DIR_ERR);
}

static void removeFileErr(const char *fileName){
    strcpy(err_name, fileName);
    longjmp(jump_buffer, REMOVE_FILE_ERR);
}

static void diyError(int code) {
    switch (code) {
        // readInfo
        case RI_CONNECT_SERVER_ERR:
            dzlog_error("0x0001FF01 连接服务器失败 (readInfo)"); break;
        case RI_SEND_ERR:
            dzlog_error("0x0001FF02 向VCU服务器发送数据失败 (readInfo)"); break;
        case RI_FRAME_HEAD_ERR:
            dzlog_error("0x0001FF03 帧头错误 (readInfo)"); break;
        case RI_DATA_HEAD_ERR:
            dzlog_error("0x0001FF04 数据头错误 (readInfo)"); break;
        case RI_NODATA_ERR:
            dzlog_error("0x0001FF05 无数据 (readInfo)"); break;
        case RI_DATA_TAIL_ERR:
            dzlog_error("0x0001FF06 数据尾错误 (readInfo)"); break;

        // downloadRequest
        case DR_CONNECT_SERVER_ERR:
            dzlog_error("0x0002FF01 连接服务器失败 (downloadRequest)"); break;
        case DR_SEND_ERR:
            dzlog_error("0x0002FF02 发送请求失败 (downloadRequest)"); break;
        case DR_FRAME_HEAD_ERR:
            dzlog_error("0x0002FF03 帧头错误 (downloadRequest)"); break;
        case DR_DATA_HEAD_ERR:
            dzlog_error("0x0002FF04 数据头错误 (downloadRequest)"); break;
        case DR_NODATA_ERR:
            dzlog_error("0x0002FF05 无数据 (downloadRequest)"); break;
        case DR_DATA_TAIL_ERROR:
            dzlog_error("0x0002FF06 数据尾错误 (downloadRequest)"); break;
        case DR_NO_MESSAGEFILE_ERR:
            dzlog_error("0x0002FF07 消息文件缺失 (downloadRequest)"); break;
        case DR_DOWNLOAD_FILE_EMPTY_ERR:
            dzlog_error("0x0002FF08 下载文件为空 (downloadRequest)"); break;

        // downloadStatus
        case DS_CONNECT_SERVER_ERR:
            dzlog_error("0x0003FF01 连接服务器失败 (downloadStatus)"); break;
        case DS_SEND_ERR:
            dzlog_error("0x0003FF02 发送失败 (downloadStatus)"); break;
        case DS_FRAME_HEAD_ERR:
            dzlog_error("0x0003FF03 帧头错误 (downloadStatus)"); break;
        case DS_DATA_HEAD_ERR:
            dzlog_error("0x0003FF04 数据头错误 (downloadStatus)"); break;
        case DS_FTPSOCKET_OPEN_ERR:
            dzlog_error("0x0003FF05 打开FTP数据连接失败 (downloadStatus)"); break;
        case DS_DATA_ERR:
            dzlog_error("0x0003FF06 数据错误 (downloadStatus)"); break;
        case DS_DATA_TAIL_ERROR:
            dzlog_error("0x0003FF07 数据尾错误 (downloadStatus)"); break;
        case DS_SUBFILE_COUNT_ERR:
            dzlog_error("0x0003FF08 子文件数量错误 (downloadStatus)"); break;
        case DS_NO_MESSAGEFILE_ERR:
            dzlog_error("0x0003FF09 消息文件缺失 (downloadStatus)"); break;
        case DS_DOWNLOAD_FILE_EMPTY_ERR:
            dzlog_error("0x0003FF0A 下载文件为空 (downloadStatus)"); break;

        // FTP
        case FTP_SOCKET_ERR:
            dzlog_error("0xFFFFFF01 创建FTP套接字失败"); break;
        case FTP_CONNECT_ERR:
            dzlog_error("0xFFFFFF02 连接FTP服务器失败"); break;
        case FTP_LOGIN_ERR:
            dzlog_error("0xFFFFFF11 登录FTP服务器失败"); break;
        case FTP_MKDIR_ERR:
            dzlog_error("0xFFFFFF21 创建FTP目录失败"); break;
        case FTP_RMDIR_ERR:
            dzlog_error("0xFFFFFF31 删除FTP目录失败"); break;
        case FTP_TYPE_ERROR:
            dzlog_error("0xFFFFFF41 设置FTP传输类型失败"); break;
        case FTP_SIZE_ERR:
            dzlog_error("0xFFFFFF51 获取FTP文件大小失败"); break;
        case FTP_PASV_ERROR:
            dzlog_error("0xFFFFFF61 设置FTP PASV模式失败"); break;
        case FTP_DATA_SOCKET_OPEN_ERR:
            dzlog_error("0xFFFFFF71 打开FTP数据套接字失败"); break;
        case FTP_D_DOWNLOAD_ERR:
            dzlog_error("0xFFFFFF81 FTP子文件下载失败"); break;
        case FTP_DOWNLOAD_ERR:
            dzlog_error("0xFFFFFF91 FTP文件下载失败"); break;
        case FTP_D_UPLOAD_ERR:
            dzlog_error("0xFFFFFFA1 FTP子文件上传失败"); break;
        case FTP_UPLOAD_ERR:
            dzlog_error("0xFFFFFFB1 FTP文件上传失败"); break;
        case GET_LOCAL_FILE_SIZE_ERR:
            dzlog_error("0xFFFFFFC1 获取本地文件大小失败"); break;
        case DOWNLOAD_RECV_ERROR:
            dzlog_error("0xFFFFFFD1 下载接收数据失败"); break;
        case UPLOAD_SEND_ERROR:
            dzlog_error("0xFFFFFFE1 上传发送数据失败"); break;

        // connectserver & 文件/目录操作
        case SOCKET_ERR:
            dzlog_error("-100 创建套接字失败 (connectserver)"); break;
        case CONNECT_ERR:
            dzlog_error("-101 连接服务器失败 (connectserver)"); break;

        case OPEN_FILE_ERR:
            dzlog_error("-201 打开%s文件失败",err_name); break;
        case WRITE_FILE_ERR:
            dzlog_error("-202 写%s文件失败",err_name); break;
        case SEEK_FILE_ERR:
            dzlog_error("-203 定位%s文件指针失败",err_name); break;
        case ACCESS_FILE_ERR:
            dzlog_error("-204 %s文件访问失败",err_name); break;
        case REMOVE_FILE_ERR:
            dzlog_error("-205 删除%s文件失败",err_name); break;

        case OPEN_DIR_ERR:
            dzlog_error("-301 打开%s目录失败",err_name); break;

        default:
            dzlog_error("未知错误码: 0x%X", code); break;
    }
}



#endif