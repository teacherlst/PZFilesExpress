#include "ftp_client.h"

extern void send_cmd(int socket_fd_p,char *cmd_p,char *user_input_p){
    char buffer_Cmd[MAX_CMD_SIZE];
    if(user_input_p == NULL){
        sprintf(buffer_Cmd,"%s\r\n",cmd_p);
        send(socket_fd_p,buffer_Cmd,strlen(buffer_Cmd),0);
    }else{
        sprintf(buffer_Cmd,"%s %s\r\n",cmd_p,user_input_p);
        send(socket_fd_p,buffer_Cmd,strlen(buffer_Cmd),0);
    }
}
extern int ftp_Connect(char *cmd_p){
    //实现ftp
    char response[1024];
    char *user_cmd_ftp = malloc(128);
    
    char username[20];
    char passwd[20];
    //char tmp_username = NULL;

    printf("正在对远程主机%s进行ftp连接...\n",hostIp);
    int control_socket = ftp_Socket(hostIp);
    if(control_socket == FTPSOCKET_OPEN_FAILURE){
        return FTP_CONNECT_FAILURE;
    }
    ftp_Login(control_socket,hostName,hostPasswd);

    int ftp_quit = false;
    int data_socket = 0;
    int flash_ftp = false;
    char *tmp_dir = NULL;
    long rest_loacl_Size = 0;
    long rest_remote_Size = 0;
    while (!ftp_quit)
    {
        char line[256]; // 用于存储输入行的缓冲区
        char *arg = NULL;  // 用于存储参数
        char *arg2 = NULL;
        strcpy(line,cmd_p);
        int total = strlen(line),i=0;
        int have_space = 0;
        while (i < total)
        {
            if(line[i] == ' '){
                have_space++;
            }
            i++;
        }
        if(have_space == 1){
            user_cmd_ftp = strtok(line," ");
            arg = strtok(NULL," ");
        }else if(have_space == 2){
            user_cmd_ftp = strtok(line," ");
            arg = strtok(NULL," ");
            arg2 = strtok(NULL," ");
        }else{
            strcpy(user_cmd_ftp,line);
        }
        if(strcmp(user_cmd_ftp,"quit") == 0){
            ftp_quit = true;
            close(control_socket);
        }
        if(strcmp(user_cmd_ftp,"pasv") == 0){
            if(data_socket == 0){
                data_socket = data_Socket_Transfer(control_socket,targetIp);
            }
        }
        if(strcmp(user_cmd_ftp,"ls") == 0){
            //

            send_cmd(control_socket,"TYPE","A");
            recv(control_socket, response, sizeof(response), 0);

            if(data_socket == 0){
                data_socket = data_Socket_Transfer(control_socket,targetIp);
            }
            send_cmd(control_socket,"LIST", NULL);
            recv(control_socket, response, sizeof(response), 0);
            //printf("%s \n",response);
            if (strncmp(response, "150", 3) != 0) {
                ftp_Error(LIST_DATA_SOCKET_OPEN_ERROR);
            }

            int a;
            while (a = recv(data_socket, response, sizeof(response), 0) > 0)
            {
                printf("%s", response);
            }
            if(data_socket != 0){
                //printf("data_socket :%d \n",data_socket);
                close(data_socket);
                data_socket = 0;
            }
            //printf("control_socket :%d \n",control_socket);
            // memset(response, 0, sizeof(response));
            // recv(control_socket, response, sizeof(response), 0);
            // //printf("bbbb %d %s \n",aaa,response);
            // if (strncmp(response, "226", 3) != 0) {
            //     ftp_Error(LIST_DATA_SOCKET_TRANSFER_ERROR);
            // }
        }
        if(strcmp(user_cmd_ftp,"pwd") == 0){
            send_cmd(control_socket,"PWD", NULL);
            recv(control_socket, response, sizeof(response), 0);
            printf("Server: %s", response);
            if (strncmp(response, "257", 3) != 0) {
                ftp_Error(PWD_DISPLAY_ERROR);
            }
        }
        if(strcmp(user_cmd_ftp,"cd") == 0){
            send_cmd(control_socket,"CWD", arg);
            recv(control_socket, response, sizeof(response), 0);
            //printf("Server: %s", response);
            if (strncmp(response, "250", 3) != 0) {
                close(control_socket);
                control_socket = 0;
                return false;
            }else{
                close(control_socket);
                control_socket = 0;
                return true;
            }
        }
        //下载
        if(strcmp(user_cmd_ftp,"download") == 0){
            long had_DownloadSize = 0;
            int progress_Download = 0;
            if(data_socket == 0){
                data_socket = data_Socket_Transfer(control_socket,targetIp);
            }
            send_cmd(control_socket,"TYPE","I");
            recv(control_socket, response, sizeof(response), 0);
            char *remote_filename = arg;
            char *local_filename = arg2;
            rest_loacl_Size = fileSize(control_socket,local_filename,LOCAL_FILE);
            rest_remote_Size = fileSize(control_socket,remote_filename,REMOTE_FILE);
            if(rest_loacl_Size != 0 && rest_loacl_Size != rest_remote_Size){
                //printf("开启断点续传\n");
                char tmp_cmd[256];
                sprintf(tmp_cmd,"REST %ld",rest_loacl_Size);
                send_cmd(control_socket,tmp_cmd, NULL);
                recv(control_socket, response, sizeof(response), 0);
                send_cmd(control_socket,"RETR", remote_filename);
                recv(control_socket, response, sizeof(response), 0);
                //开始下载文件
                had_DownloadSize = rest_loacl_Size;
                if (strncmp(response, "150", 3) == 0) {
                    int download_ret = false,try_total = 1;
                    while(!download_ret){
                        if(try_total > 3){
                            ftp_Error(FILE_DOWNLOAD_ERROR);
                        }
                        FILE *local_file = fopen(local_filename, "ab");
                        if (!local_file) {
                            printf("打开文件失败\n");
                            dzlog_info("d用户在执行-f选项 下载 时打开本地文件失败");
                        }
                        char buffer[1024];
                        ssize_t bytes_received;
                        while ((bytes_received = recv(data_socket, buffer, sizeof(buffer), 0)) > 0) {
                            fwrite(buffer, 1, bytes_received, local_file);
                            had_DownloadSize+=bytes_received;
                            progress_Download = had_DownloadSize*100/rest_remote_Size;
                            progress_Download_Or_Upload(progress_Download,DOWNLOAD);
                        }
                        if (bytes_received == -1) {
                            printf("从服务器接收数据失败\n");
                            dzlog_info("d用户在执行-f选项 下载 时从服务器接收数据失败");
                        }

                        fclose(local_file);
                        // 接收文件下载完成的响应
                        if(data_socket != 0){
                            close(data_socket);
                            data_socket = 0;
                        }
                        memset(response, 0, sizeof(response));
                        recv(control_socket, response, sizeof(response), 0);

                        FILE *is_over_file = fopen(local_filename, "r");
                        if (is_over_file == NULL) {
                            // 处理文件打开失败的情况
                            printf("打开文件失败\n");
                            dzlog_info("d用户在执行-f选项 下载 时打开本地文件检查失败");
                        }

                        fseek(is_over_file, 0, SEEK_END);  // 移动文件指针到文件末尾
                        long file_size = ftell(is_over_file);  // 获取文件大小
                        if(file_size == rest_remote_Size){
                            download_ret = true;
                            dzlog_fatal("下载时断点续传文件%s成功",remote_filename);
                        }else{
                            try_total++;
                        }
                        fclose(is_over_file);  // 关闭文件
                    }
                }
            }else{
                send_cmd(control_socket,"RETR", remote_filename);
                recv(control_socket, response, sizeof(response), 0);
                //开始下载文件
                if (strncmp(response, "150", 3) == 0) {
                    int download_ret = false,try_total = 1;
                    while(!download_ret){
                        if(try_total > 3){
                            ftp_Error(FILE_DOWNLOAD_ERROR);
                        }
                        FILE *local_file = fopen(local_filename, "wb");
                        if (!local_file) {
                            printf("打开文件失败\n");
                            dzlog_info("用户在执行-f选项 下载 时打开本地文件失败");
                        }
                        char buffer[1024];
                        ssize_t bytes_received;
                        while ((bytes_received = recv(data_socket, buffer, sizeof(buffer), 0)) > 0) {
                            fwrite(buffer, 1, bytes_received, local_file);
                            had_DownloadSize+=bytes_received;
                            progress_Download = had_DownloadSize*100/rest_remote_Size;
                            progress_Download_Or_Upload(progress_Download,DOWNLOAD);
                        }
                        if (bytes_received == -1) {
                            printf("从服务器接收数据失败\n");
                            dzlog_info("用户在执行-f选项 下载 时从服务器接收数据失败");
                        }

                        fclose(local_file);
                        // 接收文件下载完成的响应
                        if(data_socket != 0){
                            close(data_socket);
                            data_socket = 0;
                        }
                        memset(response, 0, sizeof(response));
                        recv(control_socket, response, sizeof(response), 0);
                        FILE *is_empty_file = fopen(local_filename, "r");
                        if (is_empty_file == NULL) {
                            // 处理文件打开失败的情况
                            printf("打开文件失败\n");
                            dzlog_info("用户在执行-f选项 下载 时打开本地文件检查失败");
                        }

                        fseek(is_empty_file, 0, SEEK_END);  // 移动文件指针到文件末尾
                        long file_size = ftell(is_empty_file);  // 获取文件大小
                        if(file_size != 0){
                            download_ret = true;
                            dzlog_fatal("下载文件%s成功",remote_filename);
                        }else{
                            try_total++;
                        }
                        fclose(is_empty_file);  // 关闭文件
                    }
                }
            }
            send_cmd(control_socket,"PWD",NULL);
            recv(control_socket, response, sizeof(response), 0);
            tmp_dir = tmp_Dir(response);

            control_socket = 0;
            control_socket = ftp_Socket(targetIp);
            ftp_Login(control_socket,username,passwd);   
            send_cmd(control_socket,"CWD",tmp_dir);
            recv(control_socket, response, sizeof(response), 0);
        }
        //上传
        if(strcmp(user_cmd_ftp,"upload") == 0){
            long had_UploadSize = 0;
            int progress_Upload = 0;
            send_cmd(control_socket,"TYPE","I");
            recv(control_socket, response, sizeof(response), 0);
            if(data_socket == 0){
                data_socket = data_Socket_Transfer(control_socket,hostIp);
            }
            char *remote_filename = arg2;
            char *local_filename = arg;
            rest_loacl_Size = fileSize(control_socket,local_filename,LOCAL_FILE);
            rest_remote_Size = fileSize(control_socket,remote_filename,REMOTE_FILE);
            if(rest_remote_Size != 0 && rest_loacl_Size != rest_remote_Size){
                //printf("开启断点续传%ld\n",rest_remote_Size);
                char tmp_cmd[256];
                sprintf(tmp_cmd,"REST %ld",rest_remote_Size);
                send_cmd(control_socket,tmp_cmd, NULL);
                recv(control_socket, response, sizeof(response), 0);
                send_cmd(control_socket,"STOR", remote_filename);
                recv(control_socket, response, sizeof(response), 0);
                had_UploadSize = rest_remote_Size;
                if (strncmp(response, "150", 3) == 0) {
                    int upload_ret = false,try_total = 1;
                    while(!upload_ret){
                        if(try_total > 3){
                            ftp_Error(FILE_DOWNLOAD_ERROR);
                        }
                        FILE *local_file = fopen(local_filename, "rb");
                        if (!local_file) {
                            printf("打开文件失败\n");
                            dzlog_info("d用户在执行-f选项 上传 时打开本地文件失败");
                        }
                        if (fseek(local_file, rest_remote_Size, SEEK_SET) != 0) {
                            printf("文件指针移动错误\n");
                            dzlog_info("d用户在执行-f选项 上传 时打开本地文件文件指针移动错误");
                            fclose(local_file);
                        }
                        char buffer[1024];
                        ssize_t bytes_received;
                        while ((bytes_received = fread(buffer, 1, sizeof(buffer), local_file)) > 0) {
                            send(data_socket,buffer,bytes_received,0);
                            had_UploadSize += bytes_received;
                            progress_Upload = had_UploadSize*100/rest_loacl_Size;
                            progress_Download_Or_Upload(progress_Upload,UPLOAD);
                        }
                        if (bytes_received == -1) {
                            printf("从服务器接收数据失败\n");
                            dzlog_info("d用户在执行-f选项 上传 时向服务器发送数据失败");
                        }

                        fclose(local_file);
                        // 接收文件下载完成的响应
                        if(data_socket != 0){
                            close(data_socket);
                            data_socket = 0;
                        }
                        memset(response, 0, sizeof(response));
                        recv(control_socket, response, sizeof(response), 0);
                        rest_remote_Size = fileSize(control_socket,remote_filename,REMOTE_FILE);
                        if (rest_remote_Size == rest_loacl_Size) { // 213 状态码表示SIZE命令成功
                            //printf("File exists. Size: %ld bytes.\n", file_size);
                            upload_ret = true;
                            if (rest_remote_Size == 0) {
                                printf("上传无效\n");
                                dzlog_fatal("d用户在执行-f选项 上传 无效");
                            }else{
                                return true;
                            }
                        } else {
                            printf("上传错误\n");
                            dzlog_fatal("d用户在执行-f选项 上传 时出现错误");
                            return false;
                        }
                    }
                }
            }else{
                send_cmd(control_socket,"STOR", remote_filename);
                recv(control_socket, response, sizeof(response), 0);
                //开始上传文件
                if (strncmp(response, "150", 3) == 0) {
                    int upload_ret = false,try_total = 1;
                    while(!upload_ret){
                        if(try_total > 3){
                            ftp_Error(FILE_DOWNLOAD_ERROR);
                        }
                        FILE *local_file = fopen(local_filename, "rb");
                        if (!local_file) {
                            printf("打开文件失败\n");
                            dzlog_info("用户在执行-f选项 上传 时打开本地文件失败");
                        }
                        char buffer[1024];
                        ssize_t bytes_received;
                        while ((bytes_received = fread(buffer, 1, sizeof(buffer), local_file)) > 0) {
                            send(data_socket,buffer,bytes_received,0);
                            had_UploadSize += bytes_received;
                            progress_Upload = had_UploadSize*100/rest_loacl_Size;
                            progress_Download_Or_Upload(progress_Upload,UPLOAD);
                        }
                        if (bytes_received == -1) {
                            printf("从服务器接收数据失败\n");
                            dzlog_info("用户在执行-f选项 上传 时向服务器发送数据失败");
                        }

                        fclose(local_file);
                        // 接收文件下载完成的响应
                        if(data_socket != 0){
                            close(data_socket);
                            data_socket = 0;
                        }
                        memset(response, 0, sizeof(response));
                        recv(control_socket, response, sizeof(response), 0);

                        send_cmd(control_socket,"SIZE",remote_filename);
                        recv(control_socket, response, sizeof(response), 0);
                        // 解析响应，检查文件是否存在和大小
                        int status_code;
                        long file_size;
                        sscanf(response, "%d %ld", &status_code, &file_size);

                        if (status_code == 213) { // 213 状态码表示SIZE命令成功
                            //printf("File exists. Size: %ld bytes.\n", file_size);
                            upload_ret = true;
                            if (file_size == 0) {
                                printf("上传无效\n");
                                dzlog_fatal("用户在执行-f选项 上传 无效");
                                return false;
                            }else{
                                return true;
                            }
                        } else {
                            printf("上传错误\n");
                            dzlog_fatal("用户在执行-f选项 上传 时出现错误");
                            return false;
                        }
                    }
                }
            }
            ftp_quit = true;
            close(control_socket);
            control_socket = 0;
        }
        if(strcmp(user_cmd_ftp,"mkdir") == 0){
            send_cmd(control_socket,"MKD", arg);
            recv(control_socket, response, sizeof(response), 0);
            //printf("Server: %s", response);
            if (strncmp(response, "257", 3) != 0) {
                close(control_socket);
                ftp_Error(PWD_DISPLAY_ERROR);
            }
            else{
                close(control_socket);
                control_socket = 0;
                return true;
            }
        }
        if(strcmp(user_cmd_ftp,"rmdir") == 0){
            send_cmd(control_socket,"RMD", arg);
            recv(control_socket, response, sizeof(response), 0);
            printf("Server: %s", response);
            if (strncmp(response, "250", 3) != 0) {
                ftp_Error(PWD_DISPLAY_ERROR);
            }
        }
        if(strcmp(user_cmd_ftp,"type") == 0){
            send_cmd(control_socket,"TYPE", arg);
            recv(control_socket, response, sizeof(response), 0);
            printf("Server: %s", response);
            if (strncmp(response, "200", 3) != 0) {
                ftp_Error(PWD_DISPLAY_ERROR);
            }
        }
    }
    close(control_socket);
}

extern int data_Socket_Transfer(int control_socket_p,char *targetIp_p){
    int tmp_data_socket;
    char tmp_response[1024];
    send_cmd(control_socket_p,"PASV", NULL);
    recv(control_socket_p, tmp_response, sizeof(tmp_response), 0);
    if (strncmp(tmp_response, "227", 3) != 0) {
        ftp_Error(DATA_SOCKET_GET_ERROR);
    }
    //printf("Server: %s", response);
    //passive_mode = !passive_mode;
    int passive_port_1,passive_port_2,passive_port;
    sscanf(tmp_response, "227 Entering Passive Mode (%*d,%*d,%*d,%*d,%d,%d)\r\n", &passive_port_1,&passive_port_2);
    passive_port = passive_port_1*256 + passive_port_2;
    //printf("Server_port: %d\n", passive_port);
    struct sockaddr_in server_address_A;
    server_address_A.sin_family = AF_INET;
    server_address_A.sin_port = htons(passive_port) ;
    inet_aton(targetIp_p, &server_address_A.sin_addr);
    tmp_data_socket  = socket(AF_INET, SOCK_STREAM, 0);
    if (tmp_data_socket  == -1) {
        printf("Error creating socket\n");
    }
    if (connect(tmp_data_socket , (struct sockaddr *)&server_address_A, sizeof(server_address_A)) == -1) {
        printf("Error connecting to the server\n");
        return DATASOCKET_OPEN_FAILURE;
    }
    return tmp_data_socket;
}

static void ftp_Error(int error_p){
    switch (error_p)
    {
    case LIST_DATA_SOCKET_OPEN_ERROR:
        printf("数据端口连接错误\n");
        break;
    case LIST_DATA_SOCKET_TRANSFER_ERROR:
        printf("数据端口列表数据传输错误\n");
        break;
    case PWD_DISPLAY_ERROR:
        printf("当前目录路径显示失败\n");
        break;
    case DIR_CHANGE_ERROR:
        printf("切换工作路径失败\n");
        break;
    case DATA_SOCKET_GET_ERROR:
        printf("数据端口获取失败\n");
        break;
    case FILE_DOWNLOAD_ERROR:
        printf("文件下载失败\n");
        break; 
    default:
        break;
    }
    exit(EXIT_FAILURE);
}

extern int ftp_Socket(char *targetIp_p){
    char response[1024];
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(21);
    inet_aton(targetIp_p, &server_address.sin_addr);
    int control_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (control_socket == -1) {
        printf("Error creating socket\n");
        dzlog_notice("ftp连接socket失败");
    }
    if (connect(control_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        printf("Error connecting to the server\n");
        dzlog_notice("ftp连接服务器%s失败",targetIp_p);
        // exit(EXIT_FAILURE);
        return FTPSOCKET_OPEN_FAILURE;
    }
    recv(control_socket, response, sizeof(response), 0);
    return control_socket;
}

extern int ftp_Login(int control_Socket_p,char *username_p,char *passwd_p){
    char tmp_response[1024];
    send_cmd(control_Socket_p,"USER", username_p);
    recv(control_Socket_p, tmp_response, sizeof(tmp_response), 0);
    send_cmd(control_Socket_p,"PASS", passwd_p);
    recv(control_Socket_p, tmp_response, sizeof(tmp_response), 0);
    if (strncmp(tmp_response, "230", 3) == 0) {
        return true;
    }
}

static char *tmp_Dir(char *response_p){
    char *start = NULL, *end = NULL ,*ret = NULL;
    start = strchr(response_p, '"');
    // 查找最后一个双引号
    end = strrchr(response_p, '"');

    if (start != NULL && end != NULL && start < end) {
        int length = end - start - 1;
        ret = malloc(length + 1); // 分配内存
        if (ret != NULL) {
            strncpy(ret, start + 1, length);
            ret[length] = '\0'; // 确保字符串结束
        }
    }
    return ret;
}

extern long fileSize(int control_Socket_p,char *fileName_p,int type_p){
    long file_size = 0;
    if(type_p == LOCAL_FILE){
        if (access(fileName_p, F_OK) != 0) {
            FILE *file = fopen(fileName_p, "w");
            if (file == NULL) {
                printf("Failed to open file");
            }
            return 0;
        }else{
            FILE *file = fopen(fileName_p, "rb");
            if (file == NULL) {
                printf("Failed to open file");
            }
            fseek(file, 0, SEEK_END);
            long file_size = ftell(file);
            fclose(file);
            return file_size;
        }
    }else{
        char tmp_response[1024];
        send_cmd(control_Socket_p,"SIZE",fileName_p);
        recv(control_Socket_p,tmp_response,sizeof(tmp_response),0);
        if (strncmp(tmp_response, "550", 3) == 0) {
            file_size = 0;
        }else{
            sscanf(tmp_response, "%*d %ld", &file_size);
        }
        return file_size;
    } 
}