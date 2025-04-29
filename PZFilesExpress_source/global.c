#include "global.h"
PZFilesExpressModel *model = NULL;
int trainSpeed = 0;
int sockfd = -1;
char fdlFileName[1024] = "";
SystemTime systemTime = {0};
char start_Time[64] = "";
char end_Time[64] = "";

TFDDownloadRequestFrame downloadRequestFrame = {"FDBODYS",
                                                0,
                                                NULL,
                                                "FDBODYE"};
TFDDownloadStatusFrame downloadStatusFrame = {"FDBODYS",
                                                NULL,
                                              "FDBODYE"};
TFDDownloadStatusFrame downloadStatusReceiveFrame = {"",
                                                    NULL,
                                                     ""};

char socketBuffer[MAX_SOCKET_BUFFER_SIZE];
char totalBlocks[BLOCKS_AMOUNT_IN_SUBFILE*SUBFILE_AMOUNT_MAX*sizeof(fl04_id_info_page)];
char downloadTotalBlocks[BLOCKS_AMOUNT_IN_SUBFILE*SUBFILE_AMOUNT_MAX*sizeof(fl04_id_info_page)];
char totalSubfiles[SUBFILE_AMOUNT_MAX*sizeof(TSubFile)];
// char carBlocks[BLOCKS_AMOUNT_IN_SUBFILE*SUBFILE_AMOUNT_MAX*sizeof(fl04_id_info_page)];
fl04_id_info_page *carBlocks = NULL;
int carBlocksAmount = 0;

extern int connectServer(char *vcuIp_p,int flashId_p){
    printf("开始连接远程主机:%s",vcuIp_p);
    switch (flashId_p)
    {
        case 1:
            printf(",获取--FDL--模块\n");
            break;
        case 2:
            printf(",获取--EVR--模块\n");
            break;
        default:
            break;
    }
    int ret_sockfd;
    int ret;
    ret_sockfd = socket(AF_INET,SOCK_STREAM,0);
    if(0 > ret_sockfd){
        longjmp(jump_buffer,SOCKET_ERR);
    }
    struct sockaddr_in server_addr = {0};  
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET,vcuIp_p,&server_addr.sin_addr);

    ret = connect(ret_sockfd,(struct sockaddr*)&server_addr,sizeof(server_addr));
    if(ret < 0){
        longjmp(jump_buffer,CONNECT_ERR);
        close(ret_sockfd);
    }
    printf("*********************************服务器连接成功*********************************\n");
    return ret_sockfd;
}



extern int offsetFile(const char *filename_p,char *targetStr){
    FILE *file = fopen(filename_p, "r");
    int offset = 0;
    if (file == NULL) {
        fopenFileErr(filename_p);
    }
    int tmp_strlen = strlen(targetStr);
    char buffer[tmp_strlen + 1];
    int currentOffset = 0;
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        char *foundStr = strstr(buffer, targetStr);
        if (foundStr != NULL) {
            offset = currentOffset + foundStr - buffer;
            break;
        }
        currentOffset = ftell(file);
    }

    fclose(file); 
    return offset + tmp_strlen;
}

extern int gettimeStamp(char *time){
    struct tm tm_info;
    tm_info.tm_year = atoi(strtok(time,"-"))-1900;
    tm_info.tm_mon = atoi(strtok(NULL,"-"))-1;
    tm_info.tm_mday = atoi(strtok(NULL,"_"));
    tm_info.tm_hour = atoi(strtok(NULL,"-"));
    tm_info.tm_min = atoi(strtok(NULL,"-"));
    tm_info.tm_sec = atoi(strtok(NULL,"-"));
    time_t timestamp = mktime(&tm_info);
    return timestamp;
}


extern void progress_Download_Or_Upload(int progress_p,int download_or_upload_p){
    int Ten_progress = progress_p/10;
    if(download_or_upload_p == 1){
        switch (Ten_progress)
        {
            case 0:
                printf("\r文件正在下载[#---------%d%%]",progress_p);
                break;
            case 1:
                printf("\r文件正在下载[#---------%d%%]",progress_p);
                break;
            case 2:
                printf("\r文件正在下载[##--------%d%%]",progress_p);
                break;
            case 3:
                printf("\r文件正在下载[###-------%d%%]",progress_p);
                break;
            case 4:
                printf("\r文件正在下载[####------%d%%]",progress_p);
                break;
            case 5:
                printf("\r文件正在下载[#####-----%d%%]",progress_p);
                break;
            case 6:
                printf("\r文件正在下载[######----%d%%]",progress_p);
                break;
            case 7:
                printf("\r文件正在下载[#######---%d%%]",progress_p);
                break;
            case 8:
                printf("\r文件正在下载[########--%d%%]",progress_p);
                break;
            case 9:
                printf("\r文件正在下载[#########-%d%%]",progress_p);
                break;
            case 10:
                printf("\r文件下载完成[##########%d%%]\n",progress_p);
                break;
            default:
                break;
        }
    }else{
        switch (Ten_progress)
        {
            case 0:
                printf("\r文件正在上传[#---------%d%%]",progress_p);
                break;
            case 1:
                printf("\r文件正在上传[#---------%d%%]",progress_p);
                break;
            case 2:
                printf("\r文件正在上传[##--------%d%%]",progress_p);
                break;
            case 3:
                printf("\r文件正在上传[###-------%d%%]",progress_p);
                break;
            case 4:
                printf("\r文件正在上传[####------%d%%]",progress_p);
                break;
            case 5:
                printf("\r文件正在上传[#####-----%d%%]",progress_p);
                break;
            case 6:
                printf("\r文件正在上传[######----%d%%]",progress_p);
                break;
            case 7:
                printf("\r文件正在上传[#######---%d%%]",progress_p);
                break;
            case 8:
                printf("\r文件正在上传[########--%d%%]",progress_p);
                break;
            case 9:
                printf("\r文件正在上传[#########-%d%%]",progress_p);
                break;
            case 10:
                printf("\r文件上传完成[##########%d%%]\n",progress_p);
                break;
            default:
                break;
        }
    }
    fflush(stdout);
}

extern bool isUpTime(int hour_p,int min_p,int sec_p){
    time_t currentTime;
    struct tm *timeinfo;
    time(&currentTime);
    timeinfo = localtime(&currentTime);

    int current_hour = timeinfo->tm_hour;
    int current_minute = timeinfo->tm_min;

    // 获取当天预设时间的时间戳
    struct tm target_time = *timeinfo;
    target_time.tm_hour = hour_p;  // 时(24小时制)
    target_time.tm_min = min_p;   // 分
    target_time.tm_sec = sec_p;    // 秒
    time_t target_timestamp = mktime(&target_time);

    // 获取当前时间戳
    time_t current_timestamp = mktime(timeinfo);

    // 判断是否超过当天预设时间
    if (current_timestamp > target_timestamp) {
        return true;
    } else {
        return false;
    }
}

extern int read_json_to_model(const char *filename, PZFilesExpressModel *model) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return -1;

    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    rewind(fp);

    char *data = malloc(len + 1);
    fread(data, 1, len, fp);
    data[len] = '\0';
    fclose(fp);

    cJSON *root = cJSON_Parse(data);
    free(data);
    if (!root) return -2;

    cJSON *array = cJSON_GetObjectItem(root, "AllInfo");
    if (!cJSON_IsArray(array)) {
        cJSON_Delete(root);
        return -3;
    }

    cJSON *item = cJSON_GetArrayItem(array, 0); // 只读取第一个
    if (!item) {
        cJSON_Delete(root);
        return -4;
    }

    strcpy(model->vcuIp, cJSON_GetObjectItem(item, "vcuIp")->valuestring);
    model->flashId = cJSON_GetObjectItem(item, "flashId")->valueint;
    strcpy(model->vcuName, cJSON_GetObjectItem(item, "vcuName")->valuestring);
    strcpy(model->vcuPasswd, cJSON_GetObjectItem(item, "vcuPasswd")->valuestring);
    strcpy(model->remoteHostIp, cJSON_GetObjectItem(item, "remoteHostIp")->valuestring);
    strcpy(model->remoteHostName, cJSON_GetObjectItem(item, "remoteHostName")->valuestring);
    strcpy(model->remoteHostPasswd, cJSON_GetObjectItem(item, "remoteHostPasswd")->valuestring);
    strcpy(model->fdlSaveDir, cJSON_GetObjectItem(item, "fdlSaveDir")->valuestring);
    model->hour = cJSON_GetObjectItem(item, "hour")->valueint;
    model->min = cJSON_GetObjectItem(item, "min")->valueint;
    model->sec = cJSON_GetObjectItem(item, "sec")->valueint;
    model->sleepTime = cJSON_GetObjectItem(item, "sleepTime")->valueint;
    strcpy(model->lastTime, cJSON_GetObjectItem(item, "lastTime")->valuestring);
    model->isInitDownload = cJSON_GetObjectItem(item, "isInitDownload")->valueint;

    cJSON_Delete(root);
    return 0;
}

extern int write_model_to_json(const char *filename, const PZFilesExpressModel *model) {
    cJSON *root = cJSON_CreateObject();
    cJSON *array = cJSON_CreateArray();
    cJSON *item = cJSON_CreateObject();

    cJSON_AddStringToObject(item, "vcuIp", model->vcuIp);
    cJSON_AddNumberToObject(item, "flashId", model->flashId);
    cJSON_AddStringToObject(item, "vcuName", model->vcuName);
    cJSON_AddStringToObject(item, "vcuPasswd", model->vcuPasswd);
    cJSON_AddStringToObject(item, "remoteHostIp", model->remoteHostIp);
    cJSON_AddStringToObject(item, "remoteHostName", model->remoteHostName);
    cJSON_AddStringToObject(item, "remoteHostPasswd", model->remoteHostPasswd);
    cJSON_AddStringToObject(item, "fdlSaveDir", model->fdlSaveDir);
    cJSON_AddNumberToObject(item, "hour", model->hour);
    cJSON_AddNumberToObject(item, "min", model->min);
    cJSON_AddNumberToObject(item, "sec", model->sec);
    cJSON_AddNumberToObject(item, "sleepTime", model->sleepTime);
    cJSON_AddStringToObject(item, "lastTime", model->lastTime);
    cJSON_AddNumberToObject(item, "isInitDownload", model->isInitDownload);

    cJSON_AddItemToArray(array, item);
    cJSON_AddItemToObject(root, "AllInfo", array);

    char *json_str = cJSON_Print(root);
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        cJSON_Delete(root);
        free(json_str);
        return -1;
    }

    fwrite(json_str, 1, strlen(json_str), fp);
    fclose(fp);

    cJSON_Delete(root);
    free(json_str);
    return 0;
}


extern void printModel(const PZFilesExpressModel* model) {
    if (!model) {
        printf("Model is null\n");
        return;
    }

    printf("VCU IP: %s\n", model->vcuIp);
    printf("Flash ID: %d\n", model->flashId);
    printf("VCU Name: %s\n", model->vcuName);
    printf("VCU Password: %s\n", model->vcuPasswd);
    printf("Remote Host IP: %s\n", model->remoteHostIp);
    printf("Remote Host Name: %s\n", model->remoteHostName);
    printf("Remote Host Password: %s\n", model->remoteHostPasswd);
    printf("FDL Save Directory: %s\n", model->fdlSaveDir);
    printf("Time (H:M:S): %02d:%02d:%02d\n", model->hour, model->min, model->sec);
    printf("Sleep Time: %d\n", model->sleepTime);
    printf("Last Time: %s\n", model->lastTime);
    printf("Is Init Download: %d\n", model->isInitDownload);
}

extern long getLocalFileSize(const char *fileName_p){
    if (access(fileName_p, F_OK) != 0) {
        fseekFileErr(fileName_p);
    }
    struct stat st;
    if (stat(fileName_p, &st) == 0) {
        return st.st_size;
    } else {
        longjmp(jump_buffer,GET_LOCAL_FILE_SIZE_ERR);
    }
}