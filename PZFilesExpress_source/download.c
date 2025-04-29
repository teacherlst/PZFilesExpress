#include "download.h"

static int ftp_IsLogin_Download = false;
static int ftp_TryLogin_Download = false;

extern void sendDownloadRequest(){
    int total_dr = readBlockMessage(downloadTotalBlocks,start_Time,end_Time);
    TFDFrameHead frameHead = {"FDHEADS",
                             htonl(DOWNLOADREQUEST),
                              "FDHEADE"};

    downloadRequestFrame.flashId = htonl(model->flashId);
    downloadRequestFrame.blocks = malloc(total_dr*sizeof(fl04_id_info_page));
    memset(downloadRequestFrame.blocks,0,sizeof(total_dr*sizeof(fl04_id_info_page)));
    memcpy(downloadRequestFrame.blocks,downloadTotalBlocks,total_dr*sizeof(fl04_id_info_page));

    ssize_t ret_FrameHead = send(sockfd,&frameHead,sizeof(TFDFrameHead),0);
    ssize_t ret_DownloadRequestFrame_1 = send(sockfd,downloadRequestFrame.startTag,sizeof(downloadRequestFrame.startTag),0);
    ssize_t ret_DownloadRequestFrame_2 = send(sockfd,&downloadRequestFrame.flashId,sizeof(int),0);
    ssize_t ret_DownloadRequestFrame_3 = send(sockfd,downloadRequestFrame.blocks,total_dr*sizeof(fl04_id_info_page),0);
    ssize_t ret_DownloadRequestFrame_4 = send(sockfd,downloadRequestFrame.endTag,sizeof(downloadRequestFrame.endTag),0);
    if(0 > ret_FrameHead || 0 > ret_DownloadRequestFrame_1 || 0 > ret_DownloadRequestFrame_2 
    || 0 > ret_DownloadRequestFrame_3 || 0 > ret_DownloadRequestFrame_4){
        longjmp(jump_buffer, DR_SEND_ERR);
    }
    ssize_t sendSize = ret_FrameHead + ret_DownloadRequestFrame_1 + ret_DownloadRequestFrame_2
    + ret_DownloadRequestFrame_3 + ret_DownloadRequestFrame_4;
    dzlog_debug("*********************************sendDownloadRequest 向VCU服务端发送[%ld]字节数据*********************************",sendSize);

    memset(socketBuffer,0,sizeof(char)*MAX_SOCKET_BUFFER_SIZE);
    ssize_t totalTypes = 0;
    ssize_t ret_Types = recv(sockfd,&socketBuffer,sizeof(socketBuffer),0);
    while (ret_Types != 0)
    {
        totalTypes += ret_Types;
        ret_Types = recv(sockfd,&socketBuffer[totalTypes],sizeof(socketBuffer)-totalTypes,0);
    }

    dzlog_debug("*********************************sendDownloadRequest 接收VCU服务器[%ld]字节数据*********************************",totalTypes);

    TFDFrameHead ret_TFDFrameHead_Struct;
    memcpy(&ret_TFDFrameHead_Struct,socketBuffer,sizeof(char)*20);
    if(strcmp(ret_TFDFrameHead_Struct.startTag,"FDHEADS") != 0){
        longjmp(jump_buffer, DR_FRAME_HEAD_ERR);
    }
    TFDDownloadRequestReceiveFrame downloadRequestReceiveFrame;
    memcpy(downloadRequestReceiveFrame.startTag,socketBuffer + 20,sizeof(char)*8);
    if(strcmp(downloadRequestReceiveFrame.startTag,"FDBODYS") != 0){
        longjmp(jump_buffer, DR_DATA_HEAD_ERR);
    }
    memcpy(downloadRequestReceiveFrame.path,socketBuffer + 20 + 8,sizeof(char)*PATH_LENGTH);
    //write
    int totalSubfiles_i = (totalTypes - 20 - 8 - 256 - 8)/sizeof(TSubFile);
    if(totalSubfiles_i == 0){
        longjmp(jump_buffer, DR_NODATA_ERR);
    }
    TSubFile subfile;
    int sendDownloadRequest_i_A;
    for(sendDownloadRequest_i_A = 0;sendDownloadRequest_i_A < totalSubfiles_i;sendDownloadRequest_i_A++){
        memcpy(&subfile,socketBuffer + 20 + 8 + 256 + sendDownloadRequest_i_A*sizeof(TSubFile),sizeof(TSubFile));
        getSubFileStatus(htonl(subfile.status),sendDownloadRequest_i_A + 1);
        dzlog_info("远程服务器SubFile%d路径:%s \n",sendDownloadRequest_i_A + 1,subfile.path);
        dzlog_info("远程服务器SubFile%d块总数:%d \n",sendDownloadRequest_i_A + 1,htonl(subfile.blocksAmount));
        memcpy(totalSubfiles + sendDownloadRequest_i_A*sizeof(TSubFile),socketBuffer + 20 + 8 + 256 + sendDownloadRequest_i_A*sizeof(TSubFile),sizeof(TSubFile));
    }
    memcpy(downloadRequestReceiveFrame.endTag,socketBuffer + totalTypes - 8,sizeof(char)*8);
    if(strcmp(downloadRequestReceiveFrame.endTag,"FDBODYE") != 0){
        longjmp(jump_buffer, DR_DATA_TAIL_ERROR);
    }
    writeSubfileMessageToFile(totalSubfiles,totalSubfiles_i);
    dzlog_debug("成功从服务器读取subfile信息写入信息文件");

    writeLogFile(downloadRequestReceiveFrame.path,fdlFileName);
    dzlog_debug("成功从服务器读取FDL或EVR配置文件并写入.(fdl/evr)文件");
}

static int readBlockMessage(char *downloadTotalBlocks_p,char *start_Time_p,char *end_Time_p){
    int downloadTotalBlocks_i = 0;
    int tmp_total = 0;

    int time_s = gettimeStamp(start_Time_p);
    int time_e = gettimeStamp(end_Time_p);

    if(access(MESSAGEFILENAME,F_OK) != 0){
        longjmp(jump_buffer, DR_NO_MESSAGEFILE_ERR);
    }
    char *tmp_tar = "blockAmount=";
    int offset = offsetFile(MESSAGEFILENAME,tmp_tar);
    FILE *file = fopen(MESSAGEFILENAME, "r");
    if (file == NULL) {
        fopenFileErr(MESSAGEFILENAME);
    }
    if (fseek(file, offset, SEEK_SET) != 0) {
        fseekFileErr(MESSAGEFILENAME);
        fclose(file);
    }
    fscanf(file,"%d",&tmp_total);
    int readBlockMessage_i = 0;
    for(readBlockMessage_i = 0;readBlockMessage_i < tmp_total;readBlockMessage_i++){
        if (fseek(file, offset + 2 + readBlockMessage_i*28 + readBlockMessage_i*1, SEEK_SET) != 0) {
            fseekFileErr(MESSAGEFILENAME);
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

static void writeLogFile(char *configFilePath_p,const char *localFilePath_p){
    ftp_download(configFilePath_p,localFilePath_p);
    long file_size = getLocalFileSize(localFilePath_p);
    if(file_size == 0){
        remove(localFilePath_p);
        longjmp(jump_buffer, DR_DOWNLOAD_FILE_EMPTY_ERR);
    }
    dzlog_debug("*****配置文件成功写入记录文件%s*****",localFilePath_p);
    FILE *logfile = fopen(localFilePath_p, "a");
    if (logfile == NULL) {
        fopenFileErr(localFilePath_p);
    }
    char *endLine = "\n-----end-----\n";
    fprintf(logfile,"%s",endLine);
    fclose(logfile);
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

static void writeSubfileMessageToFile(char *Subfiles_p,int totalSubfiles_p){
    FILE *fileSubfileMessage = fopen(MESSAGEFILENAME, "a");
    if (fileSubfileMessage == NULL) {
        fopenFileErr(MESSAGEFILENAME);
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
            fwriteFileErr(MESSAGEFILENAME);
        }
    }
    fclose(fileSubfileMessage);
}

extern void getTime_s_e(char *time){
    strncpy(start_Time,time,19);
    strncpy(end_Time,time + 20,19);
}

//status
extern void sendDownloadStatus(){
    deleteSubfile();
    TFDFrameHead frameHead = {"FDHEADS",
                            htonl(DOWNLOADSTATUS),
                            "FDHEADE"};
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
        longjmp(jump_buffer, DS_SEND_ERR);
    }

    ssize_t sendSize = ret_FrameHead + ret_DownloadRequestFrame_1 + ret_DownloadRequestFrame_2
    + ret_DownloadRequestFrame_3;
    dzlog_debug("*********************************sendDownloadStatus 向VCU服务端发送[%ld]字节数据*********************************",sendSize);

    ssize_t totalTypes = 0;
    ssize_t ret_Types = recv(sockfd,&socketBuffer,sizeof(socketBuffer),0);
    while (ret_Types != 0)
    {
        totalTypes += ret_Types;
        ret_Types = recv(sockfd,&socketBuffer[totalTypes],sizeof(socketBuffer)-totalTypes,0);
    }
    dzlog_debug("*********************************sendDownloadStatus 接收VCU服务器[%ld]字节数据*********************************",totalTypes);

    TFDFrameHead ret_TFDFrameHead_Struct;
    memcpy(&ret_TFDFrameHead_Struct,socketBuffer,sizeof(char)*20);
    if(strcmp(ret_TFDFrameHead_Struct.startTag,"FDHEADS") != 0){
        longjmp(jump_buffer, DS_FRAME_HEAD_ERR);
    }
    int recvSubfileAmount = (totalTypes - 20 -16)/sizeof(TSubFile);
    if(total_ds != recvSubfileAmount){
        longjmp(jump_buffer, DS_DATA_ERR);
    }
    memcpy(downloadStatusReceiveFrame.startTag,socketBuffer + 20,sizeof(char)*8);

    if(strcmp(downloadStatusReceiveFrame.startTag,"FDBODYS") != 0){
        longjmp(jump_buffer, DS_DATA_HEAD_ERR);
    }

    TSubFile recv_subfile;
    downloadStatusReceiveFrame.subfiles = malloc(recvSubfileAmount*sizeof(TSubFile));
    //实现ftp
    DownloadStatusArgs args;
    args.downloadFileAmount = 0;
    int recvSubfileAmount_i = 0;
    for(recvSubfileAmount_i = 0;recvSubfileAmount_i < recvSubfileAmount;recvSubfileAmount_i++){
        memcpy(downloadStatusReceiveFrame.subfiles + recvSubfileAmount_i*sizeof(TSubFile),socketBuffer + 20 + 8 + recvSubfileAmount_i*sizeof(TSubFile),sizeof(TSubFile));
        memcpy(&recv_subfile,socketBuffer + 20 + 8 + recvSubfileAmount_i*sizeof(TSubFile),sizeof(TSubFile));
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
            int ret_tmp = downloadFromServerSubfile(&args,fdlFileName);
            if(ret_tmp == SUCCESS){
                args.downloadFileAmount++;
            }else{
                dzlog_fatal("从VCU设备下载%s文件失败",args.configFilePath);
            }
            free(newFilename);
        }
    }
    dzlog_debug("成功从服务器读取recfile.dat文件并写入.(fdl/evr)文件");
    memcpy(downloadStatusReceiveFrame.endTag,socketBuffer + totalTypes - 8,sizeof(char)*8);
    if(strcmp(downloadStatusReceiveFrame.endTag,"FDBODYE") != 0){
        longjmp(jump_buffer, DS_DATA_TAIL_ERROR);
    }

    if(args.downloadFileAmount != total_ds){
        longjmp(jump_buffer, DS_SUBFILE_COUNT_ERR);  
    }
    printf("所有subfile文件下载完成\n");     
}


static int readSubfileMessage(char *downloadTotalSubfiles_p){
    int downloadTotalSubfiles_i = 0;
    // 使用 fopen 函数以只读模式打开文件
    if(access(MESSAGEFILENAME,F_OK) != 0){
        longjmp(jump_buffer, DS_NO_MESSAGEFILE_ERR);
    }
    //文件指针偏移量
    int offset = offsetFile(MESSAGEFILENAME,"subfileAmount=");
    //printf("offset = %d\n",offset);
    FILE *file = fopen(MESSAGEFILENAME, "r");
    if (file == NULL) {
        fopenFileErr(MESSAGEFILENAME);
    }
    if (fseek(file, offset, SEEK_SET) != 0) {
        fseekFileErr(MESSAGEFILENAME);
        fclose(file);
    }

    fscanf(file,"%d",&downloadTotalSubfiles_i);
    int readSubfileMessage_i = 0;
    for(readSubfileMessage_i = 0;readSubfileMessage_i < downloadTotalSubfiles_i;readSubfileMessage_i++){
        if (fseek(file, offset + 2 + readSubfileMessage_i*sizeof(TSubFile) + readSubfileMessage_i*1, SEEK_SET) != 0) {
            fseekFileErr(MESSAGEFILENAME);
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
        fopenDirErr(directory_path);
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
                        removeFileErr(file_path);
                    }
                }
            }
        }
    }
    closedir(dir);
}

static int downloadFromServerSubfile(DownloadStatusArgs *args_p,char *fdlFileName_p){
    ftp_download(args_p->configFilePath,args_p->localFilePath);
    //附加
    FILE *sourceFile, *destinationFile;
    char buffer[4096];
    size_t bytesRead;
    // 打开源文件
    long file_size = getLocalFileSize(args_p->localFilePath);
    if(file_size == 0){
        remove(args_p->localFilePath);
        longjmp(jump_buffer, DS_DOWNLOAD_FILE_EMPTY_ERR);
    }

    sourceFile = fopen(args_p->localFilePath, "r");
    if (sourceFile == NULL) {
        fopenFileErr(args_p->localFilePath);
    }
    // 打开目标文件，使用 "ab" 模式表示追加二进制模式
    destinationFile = fopen(fdlFileName_p, "a");
    if (destinationFile == NULL) {
        fopenFileErr(fdlFileName_p);
    }
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), sourceFile)) > 0) {
        fwrite(buffer, 1, bytesRead, destinationFile);
    }
    // 关闭文件
    fclose(sourceFile);
    fclose(destinationFile);
    getSubFileStatus(DOWNLOADED,args_p->index);
    return SUCCESS;
}