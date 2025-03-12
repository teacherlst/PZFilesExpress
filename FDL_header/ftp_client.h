#include "global.h"
#define MAX_CMD_SIZE 512
#define LIST_DATA_SOCKET_OPEN_ERROR 1
#define LIST_DATA_SOCKET_TRANSFER_ERROR 2
#define PWD_DISPLAY_ERROR 3
#define DIR_CHANGE_ERROR 4
#define DATA_SOCKET_GET_ERROR 5
#define FILE_DOWNLOAD_ERROR 7

#define LOCAL_FILE 1
#define REMOTE_FILE 2
extern void send_cmd(int socket_fd_p,char *cmd_p,char *user_input_p);
extern int ftp_Connect(char *cmd_p);

extern int data_Socket_Transfer(int control_socket_p,char *targetIp_p);
static void ftp_Error(int error_p);
extern int ftp_Socket(char *targetIp_p);
extern int ftp_Login(int control_Socket_p,char *username_p,char *passwd_p);
static char *tmp_Dir(char *response_p);
extern long fileSize(int control_Socket_p,char *fileName_p,int type_p);