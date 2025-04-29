#include "global.h"
#include "PzProcComClient.h"
#define BUFFER_SIZE 4096
typedef struct{
    int ctrlfd;
    int datafd;
}FTP_Client;
extern int ftp_connect(char *targetIp_p);
extern void ftp_login(int ctrl_sockfd,char *username_p,char *passwd_p);
extern void ftp_quit(int ctrl_sockfd);

extern bool ftp_cd(int ctrl_sockfd,const char *path);
extern void ftp_mkdir(int ctrl_sockfd,const char *dirname);
extern void ftp_rmdir(int ctrl_sockfd,const char *dirname);
extern void ftp_type(int ctrl_sockfd,const char *code);
extern void ftp_download(const char *remote_file, const char *local_file);
extern void ftp_upload(const char *local_file, const char *remote_file);
extern void ftp_enter_passive_mode(int ctrl_sock, char *ip_out, int *port_out);
extern int ftp_open_data_socket(int ctrl_sock);


// extern int ftp_Connect(char *cmd_p);
