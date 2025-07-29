#include "diyError.h"
#include "string.h"
char err_name[128] = {0};
jmp_buf jump_buffer = {0};
extern void fopenFileErr(const char *fileName){
    strcpy(err_name, fileName);
    longjmp(jump_buffer, OPEN_FILE_ERR);
}
extern void fwriteFileErr(const char *fileName){
    strcpy(err_name, fileName);
    longjmp(jump_buffer, WRITE_FILE_ERR);
}
extern void fseekFileErr(const char *fileName){
    strcpy(err_name, fileName);
    longjmp(jump_buffer, SEEK_FILE_ERR);
}
extern void fopenDirErr(const char *dirName){
    strcpy(err_name, dirName);
    longjmp(jump_buffer, OPEN_DIR_ERR);
}

extern void removeFileErr(const char *fileName){
    strcpy(err_name, fileName);
    longjmp(jump_buffer, REMOVE_FILE_ERR);
}

extern void diyError(int code) {
    switch (code) {
        // readInfo
        case RI_CONNECT_SERVER_ERR:
            dzlog_error("0x1F01 连接服务器失败 (readInfo)"); break;
        case RI_SEND_ERR:
            dzlog_error("0x1F02 向VCU服务器发送数据失败 (readInfo)"); break;
        case RI_FRAME_HEAD_ERR:
            dzlog_error("0x1F03 帧头错误 (readInfo)"); break;
        case RI_DATA_HEAD_ERR:
            dzlog_error("0x1F04 数据头错误 (readInfo)"); break;
        case RI_NODATA_ERR:
            dzlog_error("0x1F05 无数据 (readInfo)"); break;
        case RI_DATA_TAIL_ERR:
            dzlog_error("0x1F06 数据尾错误 (readInfo)"); break;

        // downloadRequest
        case DR_CONNECT_SERVER_ERR:
            dzlog_error("0x2F01 连接服务器失败 (downloadRequest)"); break;
        case DR_SEND_ERR:
            dzlog_error("0x2F02 发送请求失败 (downloadRequest)"); break;
        case DR_FRAME_HEAD_ERR:
            dzlog_error("0x2F03 帧头错误 (downloadRequest)"); break;
        case DR_DATA_HEAD_ERR:
            dzlog_error("0x2F04 数据头错误 (downloadRequest)"); break;
        case DR_NODATA_ERR:
            dzlog_error("0x2F05 无数据 (downloadRequest)"); break;
        case DR_DATA_TAIL_ERROR:
            dzlog_error("0x2F06 数据尾错误 (downloadRequest)"); break;
        case DR_NO_MESSAGEFILE_ERR:
            dzlog_error("0x2F07 消息文件缺失 (downloadRequest)"); break;
        case DR_DOWNLOAD_FILE_EMPTY_ERR:
            dzlog_error("0x2F08 下载文件为空 (downloadRequest)"); break;

        // downloadStatus
        case DS_CONNECT_SERVER_ERR:
            dzlog_error("0x3F01 连接服务器失败 (downloadStatus)"); break;
        case DS_SEND_ERR:
            dzlog_error("0x3F02 发送失败 (downloadStatus)"); break;
        case DS_FRAME_HEAD_ERR:
            dzlog_error("0x3F03 帧头错误 (downloadStatus)"); break;
        case DS_DATA_HEAD_ERR:
            dzlog_error("0x3F04 数据头错误 (downloadStatus)"); break;
        case DS_FTPSOCKET_OPEN_ERR:
            dzlog_error("0x3F05 打开FTP数据连接失败 (downloadStatus)"); break;
        case DS_DATA_ERR:
            dzlog_error("0x3F06 数据错误 (downloadStatus)"); break;
        case DS_DATA_TAIL_ERROR:
            dzlog_error("0x3F07 数据尾错误 (downloadStatus)"); break;
        case DS_SUBFILE_COUNT_ERR:
            dzlog_error("0x3F08 子文件数量错误 (downloadStatus)"); break;
        case DS_NO_MESSAGEFILE_ERR:
            dzlog_error("0x3F09 消息文件缺失 (downloadStatus)"); break;
        case DS_DOWNLOAD_FILE_EMPTY_ERR:
            dzlog_error("0x3F0A 下载文件为空 (downloadStatus)"); break;

        // FTP
        case FTP_SOCKET_ERR:
            dzlog_error("0xFF01 创建FTP套接字失败"); break;
        case FTP_CONNECT_ERR:
            dzlog_error("0xFF02 连接FTP服务器失败"); break;
        case FTP_LOGIN_ERR:
            dzlog_error("0xFF11 登录FTP服务器失败"); break;
        case FTP_MKDIR_ERR:
            dzlog_error("0xFF21 创建FTP目录失败"); break;
        case FTP_RMDIR_ERR:
            dzlog_error("0xFF31 删除FTP目录失败"); break;
        case FTP_TYPE_ERROR:
            dzlog_error("0xFF41 设置FTP传输类型失败"); break;
        case FTP_SIZE_ERR:
            dzlog_error("0xFF51 获取FTP文件大小失败"); break;
        case FTP_PASV_ERROR:
            dzlog_error("0xFF61 设置FTP PASV模式失败"); break;
        case FTP_DATA_SOCKET_OPEN_ERR:
            dzlog_error("0xFF71 打开FTP数据套接字失败"); break;
        case FTP_D_DOWNLOAD_ERR:
            dzlog_error("0xFF81 FTP子文件下载失败"); break;
        case FTP_DOWNLOAD_ERR:
            dzlog_error("0xFF91 FTP文件下载失败"); break;
        case FTP_D_UPLOAD_ERR:
            dzlog_error("0xFFA1 FTP子文件上传失败"); break;
        case FTP_UPLOAD_ERR:
            dzlog_error("0xFFB1 FTP文件上传失败"); break;
        case GET_LOCAL_FILE_SIZE_ERR:
            dzlog_error("0xFFC1 获取本地文件大小失败"); break;
        case DOWNLOAD_RECV_ERROR:
            dzlog_error("0xFFD1 下载接收数据失败"); break;
        case UPLOAD_SEND_ERROR:
            dzlog_error("0xFFE1 上传发送数据失败"); break;

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