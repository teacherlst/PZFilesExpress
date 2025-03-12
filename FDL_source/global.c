#include "global.h"
char carIp[32] = "";
int flashId = 0;
char hostIp[32] = "";
char carName[32] = "";
char carPasswd[32] = "";
char hostName[32] = "";
char hostPasswd[32] = "";
char fdlSaveDir[128] = "";
int hour = 0;
int min = 0;
int sec = 0;
int sleepTime = 0;
FDL_TO_DDU fdl_to_ddu = {0};
DDU_TO_FDL ddu_to_fdl = {0,65535};
int trainSpeed = 0;
char start_Time[64] = "";
char end_Time[64] = "";
char socketBuffer[MAX_SOCKET_BUFFER_SIZE];
char totalBlocks[BLOCKS_AMOUNT_IN_SUBFILE*SUBFILE_AMOUNT_MAX*sizeof(fl04_id_info_page)];
char downloadTotalBlocks[BLOCKS_AMOUNT_IN_SUBFILE*SUBFILE_AMOUNT_MAX*sizeof(fl04_id_info_page)];
char totalSubfiles[SUBFILE_AMOUNT_MAX*sizeof(TSubFile)];
// char carBlocks[BLOCKS_AMOUNT_IN_SUBFILE*SUBFILE_AMOUNT_MAX*sizeof(fl04_id_info_page)];
fl04_id_info_page *carBlocks = NULL;
int carBlocksAmount = 0;


extern int connectServer(char *carIp_p,int flashId_p,bool *ret_connectServer_p){
    printf("开始连接远程主机:%s",carIp_p);
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
    struct sockaddr_in server_addr = {0};  
    int ret_sockfd;
    int ret;

    ret_sockfd = socket(AF_INET,SOCK_STREAM,0);
    if(0 > ret_sockfd){
        printf("socket error\n");
        dzlog_notice("用户在连接远程主机%s时开启socket端口失败",carIp_p);
        *ret_connectServer_p = false;
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET,carIp_p,&server_addr.sin_addr);

    ret = connect(ret_sockfd,(struct sockaddr*)&server_addr,sizeof(server_addr));
    if(0 > ret){
        printf("connect error\n");
        dzlog_notice("用户连接远程主机%s失败",carIp_p);
        close(ret_sockfd);
        *ret_connectServer_p = false;
    }
    printf("服务器连接成功...\n");
    return ret_sockfd;
}



extern int offsetFile(const char *filename_p,char *targetStr){
    FILE *file = fopen(filename_p, "r");
    int offset = 0;
    if (file == NULL) {
        printf("无法打开文件 %s\n", filename_p);
        dzlog_notice("用户在使用offset函数时，打开文件%s失败",filename_p);
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

extern int isUpTime(int hour_p,int min_p,int sec_p){
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

// 从INI文件中读取字符串
extern int readStringFromINI(const char *filename, const char *section, const char *key, char *output, size_t size) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("无法打开文件: %s\n", filename);
        return 0;
    }

    char line[256];
    int found_section = 0;

    while (fgets(line, sizeof(line), file)) {
        char *trimmed_line = trim(line);

        // 检查是否是section
        if (trimmed_line[0] == '[') {
            char section_name[100];
            sscanf(trimmed_line, "[%99[^]]]", section_name);

            if (strcmp(section_name, section) == 0) {
                found_section = 1;
            } else if (found_section) {
                // 进入了另一个section，未找到key
                break;
            }
        } else if (found_section && strstr(trimmed_line, "=")) {
            char ini_key[100], ini_value[100];
            sscanf(trimmed_line, "%99[^=]=%99[^\n]", ini_key, ini_value);

            // 匹配key
            if (strcmp(trim(ini_key), key) == 0) {
                strncpy(output, trim(ini_value), size);
                fclose(file);
                return 1;  // 找到key并返回
            }
        }
    }

    fclose(file);
    return 0;
}

// 更新或写入INI文件中的字符串
extern int WriteStringToINI(const char *filename, const char *section, const char *key, const char *value) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("无法打开文件: %s\n", filename);
        return 0;
    }

    FILE *temp = fopen("temp.ini", "w");
    if (!temp) {
        printf("无法创建临时文件\n");
        fclose(file);
        return 0;
    }

    char line[256];
    int found_section = 0, key_written = 0;
    int section_written = 0;

    // 逐行读取并写入临时文件
    while (fgets(line, sizeof(line), file)) {
        char *trimmed_line = trim(line);

        // 如果到达指定的section
        if (trimmed_line[0] == '[') {
            if (found_section && !key_written) {
                // 如果找到了section但还没有写入key，写入key-value
                fprintf(temp, "%s=%s\n", key, value);
                key_written = 1;
            }

            found_section = 0;  // 结束当前section

            char section_name[100];
            sscanf(trimmed_line, "[%99[^]]]", section_name);
            if (strcmp(section_name, section) == 0) {
                found_section = 1;
            }
        }

        // 如果在指定section内并且找到了key，则更新值
        if (found_section && strstr(trimmed_line, "=")) {
            char ini_key[100];
            sscanf(trimmed_line, "%99[^=]", ini_key);
            if (strcmp(trim(ini_key), key) == 0) {
                fprintf(temp, "%s=%s\n", key, value);
                key_written = 1;
                continue;  // 跳过写入原有的行
            }
        }

        // 将原有的行写入临时文件
        fprintf(temp, "%s", line);
    }

    // 如果文件中没有找到section，添加新的section和key-value
    if (!found_section) {
        if (!section_written) {
            fprintf(temp, "\n[%s]\n", section);
            section_written = 1;
        }
        fprintf(temp, "%s=%s\n", key, value);
    } else if (!key_written) {
        fprintf(temp, "%s=%s\n", key, value);
    }

    fclose(file);
    fclose(temp);

    // 替换原始文件为临时文件
    remove(filename);
    rename("temp.ini", filename);

    return 1;
}

extern void readCarConfigFile(char *carConfigFileName_p){
    readStringFromINI(carConfigFileName_p,"Settings","carIp",carIp,sizeof(carIp));
    char tmp_FlashId;
    readStringFromINI(carConfigFileName_p,"Settings","flashId",&tmp_FlashId,sizeof(tmp_FlashId));
    flashId = atoi(tmp_FlashId);
    readStringFromINI(carConfigFileName_p,"Settings","carName",carName,sizeof(carName));
    readStringFromINI(carConfigFileName_p,"Settings","carPasswd",carPasswd,sizeof(carPasswd));
    readStringFromINI(carConfigFileName_p,"Settings","hostIp",hostIp,sizeof(hostIp));
    readStringFromINI(carConfigFileName_p,"Settings","hostName",hostName,sizeof(hostName));
    readStringFromINI(carConfigFileName_p,"Settings","hostPasswd",hostPasswd,sizeof(hostPasswd));
    char tmp_SleepTime[20];
    readStringFromINI(carConfigFileName_p,"Settings","sleepTime",tmp_SleepTime,sizeof(tmp_SleepTime));
    sleepTime = atoi(tmp_SleepTime);
    char tmp_Hour[20];
    readStringFromINI(carConfigFileName_p,"Settings","hour",tmp_Hour,sizeof(tmp_Hour));
    hour = atoi(tmp_Hour);
    char tmp_Min[20];
    readStringFromINI(carConfigFileName_p,"Settings","min",tmp_Min,sizeof(tmp_Min));
    sleepTime = atoi(tmp_Min);
    char tmp_Sec[20];
    readStringFromINI(carConfigFileName_p,"Settings","sec",tmp_Sec,sizeof(tmp_Sec));
    sleepTime = atoi(tmp_Sec);
    readStringFromINI(carConfigFileName_p,"Settings","fdlSaveDir",fdlSaveDir,sizeof(fdlSaveDir));
}