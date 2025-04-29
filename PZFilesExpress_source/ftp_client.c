#include "ftp_client.h"
extern int ftp_connect(char *targetIp_p){
    char response[BUFFER_SIZE];
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(21);
    inet_aton(targetIp_p, &server_address.sin_addr);
    int control_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (control_socket < 0) {
        longjmp(jump_buffer, FTP_SOCKET_ERR);
    }
    if (connect(control_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        longjmp(jump_buffer, FTP_CONNECT_ERR);
    }
    recv(control_socket, response, sizeof(response), 0);
    return control_socket;
}
extern void ftp_login(int ctrl_sockfd,char *username_p,char *passwd_p){
    char response[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];

    snprintf(buffer, sizeof(buffer), "USER %s\r\n", username_p);
    send(ctrl_sockfd, buffer, strlen(buffer), 0);
    recv(ctrl_sockfd, response, sizeof(response), 0);

    snprintf(buffer,sizeof(buffer),"PASS %s\r\n", passwd_p);
    send(ctrl_sockfd, buffer, strlen(buffer), 0);
    recv(ctrl_sockfd, response, sizeof(response), 0);
    if (strncmp(response, "230", 3) != 0) {
        longjmp(jump_buffer, FTP_LOGIN_ERR);
    }
}
extern void ftp_quit(int ctrl_sockfd){
    char response[BUFFER_SIZE];
    char buffer[] = "QUIT\r\n";
    send(ctrl_sockfd, buffer, strlen(buffer), 0);
}

extern bool ftp_cd(int ctrl_sockfd,const char *path){
    char response[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "CWD %s\r\n", path);
    send(ctrl_sockfd, buffer, strlen(buffer), 0);
    recv(ctrl_sockfd, response, sizeof(response), 0);
    if (strncmp(response, "250", 3) != 0) {
        return false;
    }
    return true;
}
extern void ftp_mkdir(int ctrl_sockfd,const char *dirname){
    char response[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "MKD %s\r\n", dirname);
    send(ctrl_sockfd, buffer, strlen(buffer), 0);
    recv(ctrl_sockfd, response, sizeof(response), 0);
    if (strncmp(response, "257", 3) != 0) {
        longjmp(jump_buffer, FTP_MKDIR_ERR);
    }
}

extern void ftp_rmdir(int ctrl_sockfd,const char *dirname){
    char response[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "RMD %s\r\n", dirname);
    send(ctrl_sockfd, buffer, strlen(buffer), 0);
    recv(ctrl_sockfd, response, sizeof(response), 0);
    if (strncmp(response, "250", 3) != 0) {
        longjmp(jump_buffer, FTP_RMDIR_ERR);
    }
}

extern void ftp_type(int ctrl_sockfd,const char *code){
    char response[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "TYPE %s\r\n", code);
    send(ctrl_sockfd, buffer, strlen(buffer), 0);
    recv(ctrl_sockfd, response, sizeof(response), 0);
    if (strncmp(response, "200", 3) != 0) {
        longjmp(jump_buffer, FTP_TYPE_ERROR);
    }
}

extern long ftp_size(int ctrl_sockfd,const char *filename){
    long file_size = 0;
    char response[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "SIZE %s\r\n", filename);
    send(ctrl_sockfd, buffer, strlen(buffer), 0);
    recv(ctrl_sockfd, response, sizeof(response), 0);
    if (strncmp(response, "213", 3) == 0) {
        sscanf(response, "%*d %ld", &file_size);
        return file_size;
    }else if (strncmp(response, "550", 3) == 0) {
        // 550 表示文件不存在
        return 0;
    } else {
        // 其他情况，一般认为是错误
        longjmp(jump_buffer, FTP_SIZE_ERR);
    }
}

extern void ftp_download(const char *remote_file, const char *local_file){
    char ip[64];
    char response[BUFFER_SIZE];
    long had_DownloadSize = 0;
    int progress_Download = 0;
    FTP_Client ftp_client = {0};
    ftp_client.ctrlfd = ftp_connect(model->vcuIp);
    ftp_login(ftp_client.ctrlfd,model->vcuName,model->vcuPasswd);
    ftp_client.datafd = ftp_open_data_socket(ftp_client.ctrlfd);
    ftp_type(ftp_client.ctrlfd,"I");
    long localFileSize = getLocalFileSize(local_file);
    long remoteFileSize = ftp_size(ftp_client.ctrlfd,remote_file);

    if(localFileSize != 0 && localFileSize != remoteFileSize){
        char cmd[BUFFER_SIZE];
        snprintf(cmd,sizeof(cmd),"REST %ld\r\n",localFileSize);
        send(ftp_client.ctrlfd, cmd, strlen(cmd), 0);
        recv(ftp_client.ctrlfd, response, sizeof(response), 0);

        snprintf(cmd,sizeof(cmd),"RETR %s\r\n",remote_file);
        send(ftp_client.ctrlfd, cmd, strlen(cmd), 0);
        recv(ftp_client.ctrlfd, response, sizeof(response), 0);
        had_DownloadSize = localFileSize;
        if (strncmp(response, "150", 3) == 0) {
            int download_ret = false;
            while(!download_ret){
                FILE *file = fopen(local_file, "ab");
                if (file == NULL) {
                    close(ftp_client.datafd);
                    close(ftp_client.ctrlfd);
                    fopenFileErr(local_file);
                }
                char buffer[1024];
                ssize_t bytes_received;
                while ((bytes_received = recv(ftp_client.datafd, buffer, sizeof(buffer), 0)) > 0) {
                    fwrite(buffer, 1, bytes_received, file);
                    had_DownloadSize+=bytes_received;
                    progress_Download = had_DownloadSize*100/remoteFileSize;
                    pthread_mutex_lock(&task->send_task->mutex);
                    task->send_task->pack->fdl_task_progress = progress_Download;
                    pthread_mutex_unlock(&task->send_task->mutex);
                    progress_Download_Or_Upload(progress_Download,DOWNLOAD);
                }
                if (bytes_received == -1) {
                    close(ftp_client.datafd);
                    close(ftp_client.ctrlfd);
                    longjmp(jump_buffer,DOWNLOAD_RECV_ERROR);
                }

                fclose(file);
                // 接收文件下载完成的响应
                recv(ftp_client.ctrlfd, response, sizeof(response), 0);

                long temp_file_size = getLocalFileSize(local_file);
                if(temp_file_size == remoteFileSize){
                    download_ret = true;
                    dzlog_info("下载时断点续传文件%s成功",remote_file);
                }else{
                    close(ftp_client.datafd);
                    close(ftp_client.ctrlfd);
                    longjmp(jump_buffer,FTP_D_DOWNLOAD_ERR);
                }
            }
        }
    }else{
        char cmd[BUFFER_SIZE];
        snprintf(cmd,sizeof(cmd),"RETR %s\r\n",remote_file);
        send(ftp_client.ctrlfd, cmd, strlen(cmd), 0);
        recv(ftp_client.ctrlfd, response, sizeof(response), 0);
        //开始下载文件
        if (strncmp(response, "150", 3) == 0) {
            int download_ret = false;
            while(!download_ret){
                FILE *file = fopen(local_file, "wb");
                if (file == NULL) {
                    close(ftp_client.datafd);
                    close(ftp_client.ctrlfd);
                    fopenFileErr(local_file);
                }
                char buffer[1024];
                ssize_t bytes_received;
                while ((bytes_received = recv(ftp_client.datafd, buffer, sizeof(buffer), 0)) > 0) {
                    fwrite(buffer, 1, bytes_received, file);
                    had_DownloadSize+=bytes_received;
                    progress_Download = had_DownloadSize*100/remoteFileSize;
                    pthread_mutex_lock(&task->send_task->mutex);
                    task->send_task->pack->fdl_task_progress = progress_Download;
                    pthread_mutex_unlock(&task->send_task->mutex);
                    progress_Download_Or_Upload(progress_Download,DOWNLOAD);
                }
                if (bytes_received == -1) {
                    close(ftp_client.datafd);
                    close(ftp_client.ctrlfd);
                    longjmp(jump_buffer,DOWNLOAD_RECV_ERROR);
                }

                fclose(file);
                long temp_file_size = getLocalFileSize(local_file);
                printf("temp_file_size:%ld remoteFileSize:%ld\n",temp_file_size,remoteFileSize);
                if(temp_file_size == remoteFileSize){
                    download_ret = true;
                    dzlog_info("下载文件%s成功",remote_file);
                }else{
                    close(ftp_client.datafd);
                    close(ftp_client.ctrlfd);
                    longjmp(jump_buffer,FTP_DOWNLOAD_ERR);
                }
            }
        }
    }
    ftp_quit(ftp_client.ctrlfd);
    close(ftp_client.datafd);
    close(ftp_client.ctrlfd);
}

extern void ftp_upload(const char *local_file, const char *remote_file){
    char ip[64];
    char cmd[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    long had_UploadSize = 0;
    int progress_Upload = 0;
    FTP_Client ftp_client = {0};
    ftp_client.ctrlfd = ftp_connect(model->remoteHostIp);
    ftp_login(ftp_client.ctrlfd,model->remoteHostName,model->remoteHostPasswd);

    bool ret;
    char tmp_dir[256];
    sprintf(tmp_dir,"%s%s",model->fdlSaveDir,systemTime.year);
    ret = ftp_cd(ftp_client.ctrlfd,tmp_dir);
    if(ret != true){
        ftp_mkdir(ftp_client.ctrlfd,tmp_dir);
    }

    sprintf(tmp_dir,"%s%s/%s",model->fdlSaveDir,systemTime.year,systemTime.month);
    ret = ftp_cd(ftp_client.ctrlfd,tmp_dir);
    if(ret != true){
        ftp_mkdir(ftp_client.ctrlfd,tmp_dir);
    }

    ftp_client.datafd = ftp_open_data_socket(ftp_client.ctrlfd);
    ftp_type(ftp_client.ctrlfd,"I");
    long localFileSize = getLocalFileSize(local_file);
    long remoteFileSize = ftp_size(ftp_client.ctrlfd,remote_file);
    if(remoteFileSize != 0 && localFileSize != remoteFileSize){
        snprintf(cmd,sizeof(cmd),"REST %ld\r\n",remoteFileSize);
        send(ftp_client.ctrlfd, cmd, strlen(cmd), 0);
        recv(ftp_client.ctrlfd, response, sizeof(response), 0);

        snprintf(cmd,sizeof(cmd),"STOR %s\r\n",remote_file);
        send(ftp_client.ctrlfd, cmd, strlen(cmd), 0);
        recv(ftp_client.ctrlfd, response, sizeof(response), 0);
        had_UploadSize = remoteFileSize;
        if (strncmp(response, "150", 3) == 0) {
            int upload_ret = false;
            while(!upload_ret){
                FILE *file = fopen(local_file, "rb");
                if (file == NULL) {
                    close(ftp_client.datafd);
                    close(ftp_client.ctrlfd);
                    fopenFileErr(local_file);
                }
                if (fseek(file, remoteFileSize, SEEK_SET) != 0) {
                    fclose(file);
                    close(ftp_client.datafd);
                    close(ftp_client.ctrlfd);
                    fseekFileErr(local_file);
                }
                char buffer[1024];
                ssize_t bytes_received;
                while ((bytes_received = fread(buffer, 1, sizeof(buffer), file)) > 0) {
                    send(ftp_client.datafd,buffer,bytes_received,0);
                    had_UploadSize += bytes_received;
                    progress_Upload = had_UploadSize*100/localFileSize;
                    progress_Download_Or_Upload(progress_Upload,UPLOAD);
                }
                if (bytes_received == -1) {
                    close(ftp_client.datafd);
                    close(ftp_client.ctrlfd);
                    longjmp(jump_buffer,UPLOAD_SEND_ERROR);
                }

                fclose(file);
                // 接收文件下载完成的响应
                memset(response, 0, sizeof(response));
                recv(ftp_client.ctrlfd, response, sizeof(response), 0);

                remoteFileSize = ftp_size(ftp_client.ctrlfd,remote_file);
                if (remoteFileSize == localFileSize) {
                    //printf("File exists. Size: %ld bytes.\n", file_size);
                    upload_ret = true;
                    dzlog_info("文件上传完成");
                } else {
                    close(ftp_client.datafd);
                    close(ftp_client.ctrlfd);
                    longjmp(jump_buffer,FTP_D_UPLOAD_ERR);
                }
            }
        }
    }else{
        snprintf(cmd,sizeof(cmd),"STOR %s\r\n",remote_file);
        send(ftp_client.ctrlfd, cmd, strlen(cmd), 0);
        recv(ftp_client.ctrlfd, response, sizeof(response), 0);
        //开始上传文件
        if (strncmp(response, "150", 3) == 0) {
            int upload_ret = false;
            while(!upload_ret){
                FILE *file = fopen(local_file, "rb");
                if (!file) {
                    close(ftp_client.datafd);
                    close(ftp_client.ctrlfd);
                    fopenFileErr(local_file);
                }
                char buffer[1024];
                ssize_t bytes_received;
                while ((bytes_received = fread(buffer, 1, sizeof(buffer), file)) > 0) {
                    send(ftp_client.datafd,buffer,bytes_received,0);
                    had_UploadSize += bytes_received;
                    progress_Upload = had_UploadSize*100/localFileSize;
                    progress_Download_Or_Upload(progress_Upload,UPLOAD);
                }
                if (bytes_received == -1) {
                    close(ftp_client.datafd);
                    close(ftp_client.ctrlfd);
                    longjmp(jump_buffer,UPLOAD_SEND_ERROR);
                }

                fclose(file);
                // 接收文件下载完成的响应
                memset(response, 0, sizeof(response));
                recv(ftp_client.ctrlfd, response, sizeof(response), 0);

                remoteFileSize = ftp_size(ftp_client.ctrlfd,remote_file);
                if (remoteFileSize == localFileSize) {
                    //printf("File exists. Size: %ld bytes.\n", file_size);
                    upload_ret = true;
                    dzlog_info("文件上传完成");
                } else {
                    close(ftp_client.datafd);
                    close(ftp_client.ctrlfd);
                    longjmp(jump_buffer,FTP_UPLOAD_ERR);
                }
            }
        }
    }
    ftp_quit(ftp_client.ctrlfd);
    close(ftp_client.datafd);
    close(ftp_client.ctrlfd);
}

extern void ftp_enter_passive_mode(int ctrl_sock, char *ip_out, int *port_out){
    char response[BUFFER_SIZE];
    send(ctrl_sock, "PASV\r\n", 6, 0);
    recv(ctrl_sock, response, sizeof(response), 0);

    if (strncmp(response, "227", 3) != 0) {
        longjmp(jump_buffer,FTP_PASV_ERROR);
    }
    int h1,h2,h3,h4,p1,p2;
    sscanf(strchr(response,'(')+1, "%d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2);
    snprintf(ip_out, 64, "%d.%d.%d.%d", h1, h2, h3, h4);
    *port_out = p1 * 256 + p2;
}

extern int ftp_open_data_socket(int ctrl_sock){
    char ip[64];
    int port;
    int data_socketfd;
    ftp_enter_passive_mode(ctrl_sock,ip,&port);
    struct sockaddr_in server_address_A;
    server_address_A.sin_family = AF_INET;
    server_address_A.sin_port = htons(port) ;
    inet_aton(ip, &server_address_A.sin_addr);
    data_socketfd  = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(data_socketfd , (struct sockaddr *)&server_address_A, sizeof(server_address_A)) == -1) {
        longjmp(jump_buffer,FTP_DATA_SOCKET_OPEN_ERR);
    }
    return data_socketfd;
}