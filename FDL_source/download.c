#include "download.h"
static int ftp_IsLogin_Download = false;
static int ftp_TryLogin_Download = false;
extern bool sendDownloadRequest(char *fdlFileName_p){
    bool ret_sendDownloadRequest = true;
    int total_dr = readBlockMessage(downloadTotalBlocks,start_Time,end_Time);
    //printf("%d \n",total_dr);
    int sockfd = connectServer(carIp,flashId,&ret_sendDownloadRequest);
    if(ret_sendDownloadRequest != true){
        return ret_sendDownloadRequest;
    }
    TFDFrameHead frameHead = {"FDHEADS",
                             htonl(DOWNLOADREQUEST),
                              "FDHEADE"};
    TFDDownloadRequestFrame downloadRequestFrame = {"FDBODYS",
                                                    htonl(flashId),
                                                    NULL,
                                                    "FDBODYE"};
    downloadRequestFrame.blocks = malloc(total_dr*sizeof(fl04_id_info_page));
    memcpy(downloadRequestFrame.blocks,downloadTotalBlocks,total_dr*sizeof(fl04_id_info_page));
    ssize_t ret_FrameHead = send(sockfd,&frameHead,sizeof(TFDFrameHead),0);
    ssize_t ret_DownloadRequestFrame_1 = send(sockfd,downloadRequestFrame.startTag,sizeof(downloadRequestFrame.startTag),0);
    ssize_t ret_DownloadRequestFrame_2 = send(sockfd,&downloadRequestFrame.flashId,sizeof(int),0);
    ssize_t ret_DownloadRequestFrame_3 = send(sockfd,downloadRequestFrame.blocks,total_dr*sizeof(fl04_id_info_page),0);
    ssize_t ret_DownloadRequestFrame_4 = send(sockfd,downloadRequestFrame.endTag,sizeof(downloadRequestFrame.endTag),0);
    if(0 > ret_FrameHead || 0 > ret_DownloadRequestFrame_1 || 0 > ret_DownloadRequestFrame_2 
    || 0 > ret_DownloadRequestFrame_3 || 0 > ret_DownloadRequestFrame_4){
        printf("send error\n");
        dzlog_warn("在执行sendDownloadRequest函数时，向服务器发送数据失败");
        return false;
    }
    printf("开始接收数据...\n");
    
    ssize_t totalTypes = 0;
    ssize_t ret_Types = recv(sockfd,&socketBuffer,sizeof(socketBuffer),0);
    while (ret_Types != 0)
    {
        totalTypes += ret_Types;
        ret_Types = recv(sockfd,&socketBuffer[totalTypes],sizeof(socketBuffer)-totalTypes,0);
    }
    TFDFrameHead ret_TFDFrameHead_Struct;
    memcpy(&ret_TFDFrameHead_Struct,socketBuffer,sizeof(char)*20);
    if(strcmp(ret_TFDFrameHead_Struct.startTag,"FDHEADS") != 0){
        printf("downloadRequest FrameHead receive error\n");
        dzlog_warn("在执行sendDownloadRequest函数时，接收服务器数据头帧失败");
        return false;
    }
    TFDDownloadRequestReceiveFrame downloadRequestReceiveFrame;
    memcpy(downloadRequestReceiveFrame.startTag,socketBuffer + 20,sizeof(char)*8);
    if(strcmp(downloadRequestReceiveFrame.startTag,"FDBODYS") != 0){
        printf("downloadRequest FrameHead receive error\n");
        dzlog_warn("在执行sendDownloadRequest函数时，接收服务器数据包头失败");
        return false;
    }
    memcpy(downloadRequestReceiveFrame.path,socketBuffer + 20 + 8,sizeof(char)*PATH_LENGTH);
    //printf("远程服务器配置文件路径:%s \n",downloadRequestReceiveFrame.path);
    //write
    int totalSubfiles_i = (totalTypes - 20 - 8 - 256 - 8)/sizeof(TSubFile);
    if(totalSubfiles_i == 0){
        printf("/*****无可用数据块*****/\n");
        return false;
    }
    TSubFile subfile;
    int sendDownloadRequest_i_A;
    for(sendDownloadRequest_i_A = 0;sendDownloadRequest_i_A < totalSubfiles_i;sendDownloadRequest_i_A++){
        memcpy(totalSubfiles + sendDownloadRequest_i_A*sizeof(TSubFile),socketBuffer + 20 + 8 + 256 + sendDownloadRequest_i_A*sizeof(TSubFile),sizeof(TSubFile));
        memcpy(&subfile,socketBuffer + 20 + 8 + 256 + sendDownloadRequest_i_A*sizeof(TSubFile),sizeof(TSubFile));
        getSubFileStatus(htonl(subfile.status),sendDownloadRequest_i_A + 1);
        printf("远程服务器SubFile%d路径:%s \n",sendDownloadRequest_i_A + 1,subfile.path);
        printf("远程服务器SubFile%d块总数:%d \n",sendDownloadRequest_i_A + 1,htonl(subfile.blocksAmount));
    }
    memcpy(downloadRequestReceiveFrame.endTag,socketBuffer + totalTypes - 8,sizeof(char)*8);
    if(strcmp(downloadRequestReceiveFrame.endTag,"FDBODYE") != 0){
        printf("downloadRequest FrameHead receive error\n");
        dzlog_warn("在执行sendDownloadRequest函数时，接收服务器数据包尾失败");
        return false;
    }
    writeSubfileMessageToFile(totalSubfiles,totalSubfiles_i,&ret_sendDownloadRequest);
    if(ret_sendDownloadRequest != true){
        return ret_sendDownloadRequest;
    }
    dzlog_debug("成功从服务器读取subfile信息写入信息文件");

    // char *lastPrefix = NULL;
    // char *lastSuffix = NULL;
    // char tmp_path[PATH_LENGTH];
    // if(totalSubfiles_i >= 0){
    //     lastPrefix = strrchr(subfile.path,'/');
    //     size_t prefixLength = lastPrefix - subfile.path;
    //     strncpy(tmp_path, subfile.path, prefixLength);
    //     //printf("%s\n",tmp_path);
    //     tmp_path[prefixLength] = '\0';
    //     strcat(tmp_path,"/");
    //     //strcat(tmp_path,downloadRequestReceiveFrame.path);
    //     strcat(tmp_path,downloadRequestReceiveFrame.path);
    // }
    //printf("%s\n",tmp_path);
    bool rs_writeLogFile = writeLogFile(carIp,downloadRequestReceiveFrame.path,fdlFileName_p);
    if(rs_writeLogFile == false){
        return false;
    }
    //writeLogFile(targetIp,tmp_path,fdlFileName_p);
    dzlog_debug("成功从服务器读取FDL或EVR配置文件并写入.(fdl/evr)文件");
    FILE *checkLogfile = fopen(fdlFileName_p, "r");
    if (checkLogfile == NULL) {
        printf("无法打开文件\n");
        dzlog_info("用户在执行-D r选项时，打开文件%s失败",fdlFileName_p);
        return false;
    }
    fseek(checkLogfile, 0, SEEK_END);  // 定位到文件末尾
    long size = ftell(checkLogfile);   // 获取文件大小
    if(size == 0){
        remove(fdlFileName_p);
        printf("/*****配置文件写入记录文件失败*****/\n");
        dzlog_fatal("用户在执行-D r选项时将服务器配置文件写入%s失败",fdlFileName_p);
        return false;
    }else
    {
        printf("/*****配置文件成功写入记录文件*****/\n");
    }
    
    // 关闭文件
    fclose(checkLogfile);
    close(sockfd);
    free(downloadRequestFrame.blocks);
    return ret_sendDownloadRequest;
}

// static void deleteSubfileMessage(){
//     FILE *file = fopen(MESSAGEFILENAME, "r+");
//     if (file == NULL) {
//         printf("无法打开文件 %s\n", MESSAGEFILENAME);
//         dzlog_info("用户在执行-D r选项删除Subfile信息时打开文件%s失败",MESSAGEFILENAME);
//         exit(EXIT_FAILURE);
//     }
//     //文件指针偏移量
//     int offset = offsetFile(MESSAGEFILENAME,"subfileAmount=");
//     // 定位到要删除的位置（假设删除从第10个字符开始）
//     if (fseek(file, offset - 14, SEEK_SET) != 0) {
//         printf( "无法移动文件指针\n");
//         dzlog_info("用户在执行-D r选项删除Subfile信息时文件指针偏移失败");
//         fclose(file);
//         exit(EXIT_FAILURE);
//     }
//     // 获取要删除的内容的长度
//     long deleteLength = ftell(file);
//     // 使用 ftruncate 截断文件
//     if (ftruncate(fileno(file), deleteLength) != 0) {
//         printf("无法截断文件\n");
//         dzlog_info("用户在执行-D r选项删除Subfile信息时文件截断失败");
//         fclose(file);
//         exit(EXIT_FAILURE);
//     }
//     fclose(file);
// }

static int readBlockMessage(char *downloadTotalBlocks_p,char *start_Time_p,char *end_Time_p){
    int downloadTotalBlocks_i = 0;
    int tmp_total = 0;

    int time_s = gettimeStamp(start_Time_p);
    int time_e = gettimeStamp(end_Time_p);
    if(access(MESSAGEFILENAME,F_OK) != 0){
        printf("请先执行-R选项获取块信息\n");
    }
    char *tmp_tar = "blockAmount=";
    int offset = offsetFile(MESSAGEFILENAME,tmp_tar);
    FILE *file = fopen(MESSAGEFILENAME, "r");
    if (file == NULL) {
        printf("无法打开文件 %s\n", MESSAGEFILENAME);
        dzlog_info("用户在执行-D r选项读取block信息时打开文件%s失败",MESSAGEFILENAME);
    }
    if (fseek(file, offset, SEEK_SET) != 0) {
        printf( "无法移动文件指针\n");
        dzlog_info("用户在执行-D r选项读取block信息时%s文件指针偏移失败",MESSAGEFILENAME);
        fclose(file);
    }

    fscanf(file,"%d",&tmp_total);
    int readBlockMessage_i = 0;
    for(readBlockMessage_i = 0;readBlockMessage_i < tmp_total;readBlockMessage_i++){
        if (fseek(file, offset + 2 + readBlockMessage_i*28 + readBlockMessage_i*1, SEEK_SET) != 0) {
            printf("无法移动文件指针\n");
            dzlog_info("用户在执行-D r选项读取block信息时%s文件指针偏移失败",MESSAGEFILENAME);
            fclose(file);
        }
        fl04_id_info_page blockinfo;
        memset(&blockinfo, 0, sizeof(fl04_id_info_page));
        fread(&blockinfo,sizeof(fl04_id_info_page),1,file);
            //printf("%u\n",htonl(blockinfo.first_timestamp));

        if(htonl(blockinfo.first_timestamp) >= time_s && htonl(blockinfo.first_timestamp) <= time_e){
            memcpy(downloadTotalBlocks_p + downloadTotalBlocks_i*28,&blockinfo,sizeof(fl04_id_info_page));
            downloadTotalBlocks_i += 1;
        }
    }
    fclose(file);
    return downloadTotalBlocks_i;
}

static bool writeLogFile(char *targetIp_p,char *configFilePath_p,const char *localFilePath_p){
    bool ret = true;
    long rest_remote_Size = 0;
    long had_DownloadSize = 0;
    int progress_Download = 0;
    while (ret)
    {
        //实现ftp
        int control_socket = ftp_Socket(targetIp_p);
        if(control_socket == FTPSOCKET_OPEN_FAILURE){
            return false;
        }
        char response[1024];

        ftp_Login(control_socket,carName,carPasswd);

        int data_socket = data_Socket_Transfer(control_socket,carIp);
        send_cmd(control_socket,"TYPE", "I");
        recv(control_socket, response, sizeof(response), 0);
        rest_remote_Size = fileSize(control_socket,configFilePath_p,REMOTE_FILE);
        char remote_filename[1024];
        sprintf(remote_filename, "%s %s\r\n","RETR", configFilePath_p);
        send(control_socket, remote_filename, strlen(remote_filename), 0);

        // 接收命令响应
        recv(control_socket, response, sizeof(response), 0);
        // 如果响应代码为 150，表示服务器准备好发送文件，可以开始下载
        if (strncmp(response, "150", 3) == 0) {
            // 开始下载文件
            FILE *local_file = fopen(localFilePath_p, "ab");
            if (!local_file) {
                dzlog_info("用户在执行-D r选项时writeLogFile打开本地文件%s失败",localFilePath_p);
                printf("Error opening local file for writing");
                return false;
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
                dzlog_notice("用户在执行-D r选项时writeLogFile接收远程主机内容失败");
                printf("Error receiving data from the server");
                return false;
            }

            fclose(local_file);
            // 接收文件下载完成的响应
            if(data_socket != 0){
                close(data_socket);
                data_socket = 0;
            }
            recv(control_socket, response, sizeof(response), 0);
            //printf("Server: %s\n", response);

            FILE *is_empty_file = fopen(localFilePath_p, "r");
            if (is_empty_file == NULL) {
                // 处理文件打开失败的情况
                dzlog_info("用户在执行-D r选项writeLogFile检查本地文件接收内容时打开文件失败");
                printf("Error opening file");
                return false;
            }

            fseek(is_empty_file, 0, SEEK_END);  // 移动文件指针到文件末尾
            long file_size = ftell(is_empty_file);  // 获取文件大小
            if(file_size != 0){
                ret = false;
            }
            fclose(is_empty_file);  // 关闭文件
        }
        close(data_socket);
        close(control_socket);
    }
    FILE *logfile = fopen(localFilePath_p, "a");
    if (logfile == NULL) {
        fprintf(stderr, "无法打开文件\n");
        return false;
    }
    char *endLine = "\n-----end-----\n";
    fprintf(logfile,"%s",endLine);
    fclose(logfile);
    return true;
}

static void getSubFileStatus(int status,int index){
    switch (status)
    {
    case SUSPENDING:
        printf("Subfile%d状态:SUSPENDING(初始状态)\n",index);
        break;
    case CREATING:
        printf("Subfile%d状态:CREATING(服务端文件正在生成)\n",index);
        break;
    case CREATED:
        printf("Subfile%d状态:CREATED(服务端文件已生成)\n",index);
        break;
    case DOWNLOADING:
        printf("Subfile%d状态:DOWNLOADING(客户端文件正在下载)\n",index);
        break;
    case DOWNLOADED:
        printf("Subfile%d状态:DOWNLOADED(客户端文件已被下载)\n",index);
        break;
    case DONE:
        printf("Subfile%d状态:DONE(服务端文件已被删除)\n",index);
        break;
    case CANCELING:
        printf("Subfile%d状态:CANCELING(客户端下载被取消)\n",index);
        break; 
    case CANCELED:
        printf("Subfile%d状态:CANCELED(服务端下载取消完成)\n",index);
        break; 
    default:
        break;
    }
}

static void writeSubfileMessageToFile(char *Subfiles_p,int totalSubfiles_p,bool *ret_writeSubfileMessageToFile_p){
    FILE *fileSubfileMessage = fopen(MESSAGEFILENAME, "a");
    if (fileSubfileMessage == NULL) {
        printf("无法打开文件\n");
        dzlog_info("用户在执行-D r选项时，打开文件%s失败",MESSAGEFILENAME);
        *ret_writeSubfileMessageToFile_p = false;
    }
    fprintf(fileSubfileMessage,"\nsubfileAmount=%d",totalSubfiles_p);
    TSubFile subfile;
    int writeSubfileMessageToFile_i = 0;
    for (writeSubfileMessageToFile_i = 0; writeSubfileMessageToFile_i < totalSubfiles_p; writeSubfileMessageToFile_i++) {
        fprintf(fileSubfileMessage,"\n");
        memset(&subfile, 0, sizeof(TSubFile));
        memcpy(&subfile, Subfiles_p + writeSubfileMessageToFile_i * sizeof(TSubFile), sizeof(TSubFile));
        size_t writeSize = fwrite(&subfile, sizeof(TSubFile), 1, fileSubfileMessage);
        if (writeSize != 1) {
            printf("文件写入错误\n");
            dzlog_info("用户在执行-D r选项时，将块信息写入文件%s失败",MESSAGEFILENAME);
            *ret_writeSubfileMessageToFile_p = false;
        }
    }
    fclose(fileSubfileMessage);
}

extern void getTime_s_e(char *time){
    strncpy(start_Time,time,19);
    strncpy(end_Time,time + 20,19);
}

extern void writeMesssageToConfigFile(char *srcFile,char *srcStr,char *tarStr){
    int offset = offsetFile(srcFile,srcStr);
    if(offset != strlen(srcStr)){
        FILE *file = fopen(srcFile, "r+");
        if (file == NULL) {
            printf("无法打开文件 %s\n", srcFile);
            dzlog_info("用户在执行-D r选项写入fdl文件名信息时打开文件%s失败",srcFile);
            exit(EXIT_FAILURE);
        }
        //文件指针偏移量
        // 定位到要删除的位置（假设删除从第10个字符开始）
        if (fseek(file, offset - strlen(srcStr), SEEK_SET) != 0) {
            printf( "无法移动文件指针\n");
            dzlog_info("用户在执行-D r选项写入fdl文件名时文件指针偏移失败");
            fclose(file);
            exit(EXIT_FAILURE);
        }
        // 获取要删除的内容的长度
        long deleteLength = ftell(file);
        // 使用 ftruncate 截断文件
        if (ftruncate(fileno(file), deleteLength) != 0) {
            printf("无法截断文件\n");
            dzlog_info("用户在执行-D r选项删除Subfile信息时文件截断失败");
            fclose(file);
            exit(EXIT_FAILURE);
        }
        fclose(file);
    }
    FILE *configFile = fopen(srcFile,"a");
    if(configFile == NULL){
        printf("打开config.ini配置文件失败\n");
        dzlog_info("用户在执行-t选项时，写入配置文件%s失败",srcFile);
        exit(EXIT_FAILURE);
    }
    fprintf(configFile,srcStr,NULL);
    fprintf(configFile,"%s\n",tarStr);
    fclose(configFile);
}

//status
extern bool sendDownloadStatus(char *fdlFileName_p){
    deleteSubfile();
    deleteSubfileLog(fdlFileName_p);
    bool ret_sendDownloadStatus = true;
    //printf("%d \n",total_dr);

    char tmp_Response[1024];
    ThreadArgs args;
    strcpy(args.targetIp,carIp);
    //socket
    int sockfd = connectServer(targetIp,flashId_C,&ret_sendDownloadStatus);
    if(ret_sendDownloadStatus != true){
        return ret_sendDownloadStatus;
    }
    TFDFrameHead frameHead = {"FDHEADS",
                            htonl(DOWNLOADSTATUS),
                            "FDHEADE"};
    TFDDownloadStatusFrame downloadStatusFrame = {"FDBODYS",
                                                    NULL,
                                                "FDBODYE"};
    int total_ds = readSubfileMessage(totalSubfiles);
    //判断   
    downloadStatusFrame.subfiles = malloc(total_ds*sizeof(TSubFile));
    memcpy(downloadStatusFrame.subfiles,totalSubfiles,total_ds*sizeof(TSubFile));
    ssize_t ret_FrameHead = send(sockfd,&frameHead,sizeof(TFDFrameHead),0);
    ssize_t ret_DownloadRequestFrame_1 = send(sockfd,downloadStatusFrame.startTag,sizeof(downloadStatusFrame.startTag),0);
    ssize_t ret_DownloadRequestFrame_2 = send(sockfd,downloadStatusFrame.subfiles,total_ds*sizeof(TSubFile),0);
    ssize_t ret_DownloadRequestFrame_3 = send(sockfd,downloadStatusFrame.endTag,sizeof(downloadStatusFrame.endTag),0);
    if(0 > ret_FrameHead || 0 > ret_DownloadRequestFrame_1 || 0 > ret_DownloadRequestFrame_2 
    || 0 > ret_DownloadRequestFrame_3){
        printf("send error\n");
        dzlog_warn("在执行sendDownloadStatus函数时，向服务器发送数据失败");
        return false;
    }
    printf("开始接收数据...\n");
    
    ssize_t totalTypes = 0;
    ssize_t ret_Types = recv(sockfd,&socketBuffer,sizeof(socketBuffer),0);
    while (ret_Types != 0)
    {
        totalTypes += ret_Types;
        ret_Types = recv(sockfd,&socketBuffer[totalTypes],sizeof(socketBuffer)-totalTypes,0);
    }
    //printf("接收的总字节数:%ld \n",totalTypes);
    TFDFrameHead ret_TFDFrameHead_Struct;
    memcpy(&ret_TFDFrameHead_Struct,socketBuffer,sizeof(char)*20);
    if(strcmp(ret_TFDFrameHead_Struct.startTag,"FDHEADS") != 0){
        printf("downloadStatus FrameHead receive error\n");
        dzlog_warn("在执行sendDownloadStatus函数时，接收服务器数据头帧失败");
        return false;
    }
    int recvSubfileAmount = (totalTypes - 20 -16)/sizeof(TSubFile);

    if(total_ds != recvSubfileAmount){
        printf("receive error \n");
        return false;
    }
    TFDDownloadStatusFrame downloadStatusReceiveFrame;
    memcpy(downloadStatusReceiveFrame.startTag,socketBuffer + 20,sizeof(char)*8);

    if(strcmp(downloadStatusReceiveFrame.startTag,"FDBODYS") != 0){
        printf("downloadStatus FrameHead receive error\n");
        dzlog_warn("在执行sendDownloadStatus函数时，接收服务器数据包头失败");
        return false;
    }

    TSubFile recv_subfile;
    downloadStatusReceiveFrame.subfiles = malloc(recvSubfileAmount*sizeof(TSubFile));
    //实现ftp
    
    char response[1024];
    args.downloadFileAmount = 0;
    int recvSubfileAmount_i = 0;
    for(recvSubfileAmount_i = 0;recvSubfileAmount_i < recvSubfileAmount;recvSubfileAmount_i++){
        memcpy(downloadStatusReceiveFrame.subfiles + recvSubfileAmount_i*sizeof(TSubFile),socketBuffer + 20 + 8 + recvSubfileAmount_i*sizeof(TSubFile),sizeof(TSubFile));
        memcpy(&recv_subfile,socketBuffer + 20 + 8 + recvSubfileAmount_i*sizeof(TSubFile),sizeof(TSubFile));
        args.ftp_ControlSocket = ftp_Socket(targetIp);
        ftp_Login(args.ftp_ControlSocket,carName,carPasswd);
        //printf("%s %s\n",username,passwd);

        printf("服务器端");
        getSubFileStatus(htonl(recv_subfile.status) ,recvSubfileAmount_i + 1);
        //printf("SubFile%d路径:%s \n",i + 1,recv_subfile.path);
        //printf("SubFile%d块总数:%d \n",i + 1,htonl(recv_subfile.blocksAmount)); 
        if(htonl(recv_subfile.status) == CREATED){
            strcpy(args.configFilePath,recv_subfile.path);
            char *lastPrefix = NULL;
            lastPrefix = strrchr(recv_subfile.path,'/');
            int newLength = 1 + strlen(lastPrefix) + 1;
            char *newFilename = (char *)malloc(newLength);
            strcpy(newFilename, ".");
            strcat(newFilename, lastPrefix);
            strcpy(args.localFilePath,newFilename);
            args.index = recvSubfileAmount_i + 1;
            downloadFromServerSubfile(&args,fdlFileName_p);
            free(newFilename);
        }
    }
    dzlog_debug("成功从服务器读取recfile.dat文件并写入.(fdl/evr)文件");

    memcpy(downloadStatusReceiveFrame.endTag,socketBuffer + totalTypes - 8,sizeof(char)*8);
    if(strcmp(downloadStatusReceiveFrame.endTag,"FDBODYE") != 0){
        printf("downloadRequest FrameHead receive error\n");
        dzlog_warn("在执行sendDownloadRequest函数时，接收服务器数据包尾失败");
        return false;
    }

    if(args.downloadFileAmount != total_ds){
        printf("subfile文件下载失败或丢失\n"); 
        return false;  
    }
    printf("所有subfile文件下载完成\n");     
    close(sockfd);
    free(downloadStatusFrame.subfiles);
    free(downloadStatusReceiveFrame.subfiles);

    return ret_sendDownloadStatus;
}


static int readSubfileMessage(char *downloadTotalSubfiles_p){
    int downloadTotalSubfiles_i = 0;
    // 使用 fopen 函数以只读模式打开文件
    if(access(MESSAGEFILENAME,F_OK) != 0){
        printf("请先执行-R选项获取块信息\n");
    }
    //文件指针偏移量
    int offset = offsetFile(MESSAGEFILENAME,"subfileAmount=");
    //printf("offset = %d\n",offset);
    FILE *file = fopen(MESSAGEFILENAME, "r");
    if (file == NULL) {
        printf("无法打开文件 %s\n", MESSAGEFILENAME);
        dzlog_info("用户在执行-D s选项读取Subfile信息时打开文件%s失败",MESSAGEFILENAME);
    }
    if (fseek(file, offset, SEEK_SET) != 0) {
        printf("无法移动文件指针\n");
        dzlog_info("用户在执行-D s选项读取Subfile信息时文件指针偏移失败");
        fclose(file);
    }

    fscanf(file,"%d",&downloadTotalSubfiles_i);
    int readSubfileMessage_i = 0;
    for(readSubfileMessage_i = 0;readSubfileMessage_i < downloadTotalSubfiles_i;readSubfileMessage_i++){
        if (fseek(file, offset + 2 + readSubfileMessage_i*sizeof(TSubFile) + readSubfileMessage_i*1, SEEK_SET) != 0) {
            printf("无法移动文件指针\n");
            dzlog_info("用户在执行-D s选项读取Subfile信息时文件指针偏移失败");
            fclose(file);
        }
        TSubFile subfile;
        memset(&subfile, 0, sizeof(subfile));
        fread(&subfile,sizeof(subfile),1,file);
        memcpy(downloadTotalSubfiles_p + readSubfileMessage_i*sizeof(TSubFile),&subfile,sizeof(TSubFile));
    }
    fclose(file);
    return downloadTotalSubfiles_i;
}

static void deleteSubfile(){
    const char *directory_path = "."; // 当前目录

    DIR *dir = opendir(directory_path);
    if (dir == NULL) {
        perror("无法打开目录");
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        char file_path[512];
        snprintf(file_path, sizeof(file_path), "%s/%s", directory_path, entry->d_name);

        struct stat file_stat;
        if (stat(file_path, &file_stat) == 0) {
            if (S_ISREG(file_stat.st_mode)) { // 使用 S_ISREG 宏判断是否是普通文件
                if (strstr(entry->d_name, ".dat") != NULL) { // 判断是否以 ".dat" 后缀结尾
                    if (remove(file_path) == 0) {
                        //printf("文件 %s 已删除\n", file_path);
                    } else {
                        perror("无法删除文件");
                    }
                }
            }
        }
    }

    closedir(dir);
}

static void deleteSubfileLog(char *fdlFileName_p){
    FILE *file = fopen(fdlFileName_p, "r+");
    if (file == NULL) {
        printf("无法打开文件 %s\n", fdlFileName_p);
        dzlog_info("用户在执行-D s选项deleteSubfileLog函数时打开文件%s失败",fdlFileName_p);
    }
    //文件指针偏移量
    int offset = offsetFile(fdlFileName_p,"-----end-----\n");
    // 定位到要删除的位置（假设删除从第10个字符开始）
    if (fseek(file, offset, SEEK_SET) != 0) {
        printf("无法移动文件指针\n");
        dzlog_info("用户在执行-D s选项deleteSubfileLog函数时失败");
        fclose(file);
    }
    // 获取要删除的内容的长度
    long deleteLength = ftell(file);
    // 使用 ftruncate 截断文件
    if (ftruncate(fileno(file), deleteLength) != 0) {
        printf("无法截断文件\n");
        dzlog_info("用户在执行-D s选项deleteSubfileLog函数时文件截断失败");
        fclose(file);
    }
    fclose(file);
}

static void downloadFromServerSubfile(ThreadArgs *args_p,char *fdlFileName_p){
    int ret = true;
    char ftp_command[1024];
    long rest_remote_Size = 0;
    long had_DownloadSize = 0;
    int progress_Download = 0;
    while (ret)
    {
        getSubFileStatus(DOWNLOADING,args_p->index);
        char response[1024];
        int data_socket = data_Socket_Transfer(args_p->ftp_ControlSocket,targetIp);
        send_cmd(args_p->ftp_ControlSocket,"TYPE","I");
        recv(args_p->ftp_ControlSocket, response, sizeof(response), 0);

        rest_remote_Size = fileSize(args_p->ftp_ControlSocket,args_p->configFilePath,REMOTE_FILE);
        char remote_filename[1024];
        sprintf(remote_filename, "%s %s\r\n","RETR", args_p->configFilePath);
        send(args_p->ftp_ControlSocket, remote_filename, strlen(remote_filename), 0);
        recv(args_p->ftp_ControlSocket, response, sizeof(response), 0);

        // 如果响应代码为 150，表示服务器准备好发送文件，可以开始下载
        if (strncmp(response, "150", 3) == 0) {
            // 开始下载文件
            FILE *local_file = fopen(args_p->localFilePath, "wb");
            if (!local_file) {
                dzlog_info("用户在执行-D s选项时downloadFromServerSubfile打开本地文件%s失败",args_p->localFilePath);
                printf("Error opening local file for writing");
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
                dzlog_notice("用户在执行-D s选项时downloadFromServerSubfile接收远程主机内容失败");
                printf("Error receiving data from the server");
            }
            
            fclose(local_file);
            // 接收文件下载完成的响应
            if(data_socket != 0){
                close(data_socket);
                data_socket = 0;
            }
            recv(args_p->ftp_ControlSocket, response, sizeof(response), 0);
            //printf("Server: %s", response);

            FILE *is_empty_file = fopen(args_p->localFilePath, "r");
            if (is_empty_file == NULL) {
                // 处理文件打开失败的情况
                dzlog_info("用户在执行-D s选项downloadFromServerSubfile检查本地文件接收内容时打开文件失败");
                printf("Error opening file");
            }

            fseek(is_empty_file, 0, SEEK_END);  // 移动文件指针到文件末尾
            long file_size = ftell(is_empty_file);  // 获取文件大小
            if(file_size != 0){
                ret = false;
                args_p->downloadFileAmount++;
            }
            fclose(is_empty_file);  // 关闭文件
        }
        close(data_socket);
        close(args_p->ftp_ControlSocket);
    }
    //附加
    FILE *sourceFile, *destinationFile;
    char buffer[4096];
    size_t bytesRead;
    // 打开源文件
    sourceFile = fopen(args_p->localFilePath, "r");
    if (sourceFile == NULL) {
        perror("无法打开源文件");
    }
    // 打开目标文件，使用 "ab" 模式表示追加二进制模式
    destinationFile = fopen(fdlFileName_p, "a");
    if (destinationFile == NULL) {
        perror("无法打开目标文件");
    }
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), sourceFile)) > 0) {
        fwrite(buffer, 1, bytesRead, destinationFile);
    }
    // 关闭文件
    fclose(sourceFile);
    fclose(destinationFile);
    getSubFileStatus(DOWNLOADED,args_p->index);
}