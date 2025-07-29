/*
 * Copyright (c) [2025] [Liu Shitong Liu Ye]
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * version:测试版:0.0.4
 * date:2025-05-07
 */
#include "global.h"
#include "readInfo.h"
#include "download.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <fcntl.h>  // 用于非阻塞模式
#include "PzTypes.h"
#include "PzProcComClient.h"

#define CONSECUTIVE_PINGS 5
#define MAX_RETRY 3

static void warningSpeed(pz_uint16_t speed)
{
	printf("train speed is %d\n",speed);
}

static void recvManCmd(pz_uint8_t cmd)
{
	printf("there is cmd %d\n",cmd);
}
static int directory_exists(const char *path);
static void deleteFdlFiles(const char* dirPath);
static uint16_t crc16_false(unsigned char *puchMsg, unsigned int usDataLen);
static void downloadFileFunc(DownloadThreadArgs *arg);
static char lastTime[256] = "";
static u_int8_t fdlDownloadProgress = 0x55;
static DownloadThreadArgs DlThreadArgs = {false,false};
static pz_uint8_t oldManualCmd = false;
int main(int argc,char *argv[]){
    int zlogId = dzlog_init("./zlog.conf", "PZFilesExpress");
    if (zlogId) {
        printf("init failed, please check file %s.\n", "zlog.conf");
        exit(EXIT_FAILURE);
    }

    model = (PZFilesExpressModel *)malloc(sizeof(PZFilesExpressModel));
    bzero(model,sizeof(PZFilesExpressModel));
    printf("读取车辆配置文件..\n");
    read_json_to_model(CFGFILENAME,model);
    printModel(model);

    printf("start init cli task..\n");
    initCliTask("127.0.0.1",warningSpeed,recvManCmd);//server ip
    printf("init cli task finished..\n");
    startCliTask();
    
    bool m_totayDownload = false;
    char currentTime[256];//
    int consecutive_pings = 0;
    int tryTotal = 1;
    pthread_t thread_id;
    while (true) {
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        sprintf(currentTime,"%04d/%02d/%02d-%02d:%02d:%02d",
            tm.tm_year+1900,tm.tm_mon+1,tm.tm_mday,
            tm.tm_hour,tm.tm_min,tm.tm_sec);
        read_json_to_model(CFGFILENAME,model);
        if(model->day != tm.tm_mday){
            m_totayDownload = false;
        }else{
            m_totayDownload = true;
        }
        char command[256];
        //sprintf(command, "ping -c 1 192.168.0.1 > /dev/null");
        sprintf(command, "ping -c 1 %s > /dev/null",model->vcuIp);
        int ping_result = system(command);
        if(ping_result == 0){
            consecutive_pings++;
            printf("manual_cmd : [%d] oldManualCmd : [%d] ",task->recv_task->pack->manual_cmd,oldManualCmd);
            printf("TrainNum : [%u] CabNum : [%u]\n",getSrvTrainNum(),getCabNum());
            bool autoRet = isUpTime(model->hour,model->min,model->sec) && (m_totayDownload == false);
            bool manualRet = (task->recv_task->pack->manual_cmd == true) && (oldManualCmd == false);
            if (consecutive_pings >= CONSECUTIVE_PINGS 
                && !DlThreadArgs.thread_started 
                && (autoRet || manualRet)
                && getTrainSpeed() == 0
                && getSrvTrainNum() != 0){
                if(getSrvTrainNum() > 0 && getSrvTrainNum() < 10){
                    sprintf(model->remoteHostName,"NJL50%u0%u",getSrvTrainNum(),getCabNum());
                }else{
                    sprintf(model->remoteHostName,"NJL5%u0%u",getSrvTrainNum(),getCabNum());
                }
                dzlog_debug("第%d次下载开始",tryTotal);
                downloadFileFunc(&DlThreadArgs);
                if(DlThreadArgs.threadResult == true){
                    consecutive_pings = 0;
                    DlThreadArgs.threadResult = false;
                    dzlog_debug("当天文件处理成功");
                    model->day = tm.tm_mday;
                    write_model_to_json(CFGFILENAME,model);
                    tryTotal = 1;
                }else if(DlThreadArgs.threadResult == false){
                    dzlog_debug("第%d次下载失败",tryTotal);
                    tryTotal++;
                }
                if(tryTotal > MAX_RETRY){
                    dzlog_debug("当天下载失败");
                    model->day = tm.tm_mday;
                    write_model_to_json(CFGFILENAME,model);
                    consecutive_pings = 0;
                    tryTotal = 1;
                }
            }
            oldManualCmd = task->recv_task->pack->manual_cmd;
        }else{
            consecutive_pings = 0;
        }
        sleep(1);
    }
    free(model);
    zlog_fini();
    return 0;
}
static int directory_exists(const char *path) {
    struct stat info;
    if (stat(path, &info) != 0) {
        return 0; 
    }
    return S_ISDIR(info.st_mode);
}

static void downloadFileFunc(DownloadThreadArgs *args) {
    // DownloadThreadArgs *args = (DownloadThreadArgs *)arg;
    char timeStr[256];
    char blockMaxTime[256];
    char blockMinTime[256];
    char timeRange[514];
    // 在这里执行你想要执行的函数
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    sprintf(timeStr,"%04d-%02d-%02d_%02d-%02d-%02d",
        tm.tm_year+1900,tm.tm_mon+1,tm.tm_mday,
        tm.tm_hour,tm.tm_min,tm.tm_sec);
    sprintf(systemTime.year,"%04d",tm.tm_year+1900);
    sprintf(systemTime.month,"%02d",tm.tm_mon+1);
    sprintf(systemTime.day,"%02d",tm.tm_mday);

    int m_ret = setjmp(jump_buffer);
    if (m_ret != SUCCESS) {
        diyError(m_ret);
        dzlog_debug("-----------------------------%s-%s-%s end------------------------------",systemTime.year,systemTime.month,systemTime.day);
        args->thread_started = false;
        args->threadResult = false;
        return NULL;
    }
    dzlog_debug("-----------------------------%s-%s-%s start------------------------------",systemTime.year,systemTime.month,systemTime.day);
    char log_Path[50] = "./log";
    if (!directory_exists(log_Path)) {
        if (mkdir(log_Path, 0777) == -1) {
            dzlog_error("创建日志文件夹失败");
        }
    }

    deleteFdlFiles(log_Path);

    sockfd = connectServer(model->vcuIp,model->flashId);
    if(sockfd <= 0){
        longjmp(jump_buffer,RI_CONNECT_SERVER_ERR);
    }
    sendReadInfo();
    close(sockfd);

    fl04_id_info_page blockinfo;
    memset(&blockinfo, 0, sizeof(fl04_id_info_page));
    memcpy(&blockinfo, totalBlocks, sizeof(fl04_id_info_page));
    unsigned int max_first_timestamp = htonl((blockinfo).first_timestamp);
    unsigned int min_first_timestamp = htonl((blockinfo).first_timestamp);

    int thread_function_i = 0;
    for (thread_function_i = 0; thread_function_i < blockTotal - 1; thread_function_i++) {
        memset(&blockinfo, 0, sizeof(fl04_id_info_page));
        memcpy(&blockinfo, totalBlocks + (thread_function_i+1)* sizeof(fl04_id_info_page), sizeof(fl04_id_info_page));
        if(max_first_timestamp < htonl((blockinfo).first_timestamp)){
            max_first_timestamp = htonl((blockinfo).first_timestamp);
        }
        if(min_first_timestamp > htonl((blockinfo).first_timestamp)){
            min_first_timestamp = htonl((blockinfo).first_timestamp);
        }
    }

    time_t timetamp_max_first_timestamp = max_first_timestamp;
    struct tm tm_max_first_timestamp = *localtime(&timetamp_max_first_timestamp);
    sprintf(blockMaxTime,"%04d-%02d-%02d_%02d-%02d-%02d",
            tm_max_first_timestamp.tm_year+1900,tm_max_first_timestamp.tm_mon+1,tm_max_first_timestamp.tm_mday,
            tm_max_first_timestamp.tm_hour,tm_max_first_timestamp.tm_min,tm_max_first_timestamp.tm_sec);
    
    time_t timetamp_min_first_timestamp = min_first_timestamp;
    struct tm tm_min_first_timestamp = *localtime(&timetamp_min_first_timestamp);
    sprintf(blockMinTime,"%04d-%02d-%02d_%02d-%02d-%02d",
            tm_min_first_timestamp.tm_year+1900,tm_min_first_timestamp.tm_mon+1,tm_min_first_timestamp.tm_mday,
            tm_min_first_timestamp.tm_hour,tm_min_first_timestamp.tm_min,tm_min_first_timestamp.tm_sec);
    dzlog_debug("**********块最大时间：%s 块最小时间：%s**********",blockMaxTime,blockMinTime);
    //printf("%s %s \n",blockMaxTime,blockMinTime);
    if(model->isInitDownload == false){
        sprintf(timeRange,"%s,%s",blockMinTime,blockMaxTime);
    }else{
        read_json_to_model(CFGFILENAME,model);
        //块上次时间到最大时间
        sprintf(timeRange,"%s,%s",model->lastTime,blockMaxTime);
    }
    sprintf(fdlFileName,"%s/%s(%s).fdl",log_Path,timeRange,timeStr);
    dzlog_debug("**********块范围：%s**********",timeRange);
    getTime_s_e(timeRange);

    sockfd = connectServer(model->vcuIp,model->flashId);
    if(sockfd <= 0){
        longjmp(jump_buffer,DR_CONNECT_SERVER_ERR);
    }
    sendDownloadRequest();
    free(downloadRequestFrame.blocks);
    close(sockfd);

    sockfd = connectServer(model->vcuIp,model->flashId);
    if(sockfd <= 0){
        longjmp(jump_buffer,DS_CONNECT_SERVER_ERR);
    }
    sendDownloadStatus();
    free(downloadStatusFrame.subfiles);
    free(downloadStatusReceiveFrame.subfiles);
    close(sockfd);
    
    char remote_file_name[BUFFER_SIZE];
    sprintf(remote_file_name,"%s%s/%s/%s(%s)%s",model->fdlSaveDir,systemTime.year,systemTime.month,timeRange,timeStr,".fdl");
    ftp_upload(fdlFileName,remote_file_name);
    if(carBlocks != NULL){
        free(carBlocks);
    }

    if(model->isInitDownload == false){
        model->isInitDownload = true;
        write_model_to_json(CFGFILENAME,model);
    }
    bzero(model->lastTime,sizeof(model->lastTime));
    strcpy(model->lastTime,blockMaxTime);
    write_model_to_json(CFGFILENAME,model);

    dzlog_debug("当天传输成功");
    dzlog_debug("-----------------------------%s-%s-%s end------------------------------",systemTime.year,systemTime.month,systemTime.day);
    args->threadResult = true;
    args->thread_started = false;
}

static uint16_t crc16_false(unsigned char *puchMsg, unsigned int usDataLen)
{
    unsigned short wCRCin = 0xFFFF;
    unsigned short wCPoly = 0x1021;
    unsigned char wChar = 0;
    while (usDataLen--)
    {
        wChar = *(puchMsg++);
        wCRCin ^= (wChar << 8);
        int i;
        for(i = 0;i < 8;i++)
        {
            if(wCRCin & 0x8000)
                wCRCin = (wCRCin << 1) ^ wCPoly;
            else
                wCRCin = wCRCin << 1;
        }
    }
    return (wCRCin) ;
}


static void deleteFdlFiles(const char* dirPath) {
    DIR *dir = opendir(dirPath);
    if (!dir) {
        perror("打开目录失败");
        return;
    }

    struct dirent *entry;
    char filePath[256];

    while (entry = readdir(dir)) {
        if (entry->d_type == DT_REG) { // 只处理普通文件
            const char *ext = strrchr(entry->d_name, '.');
            if (ext && strcmp(ext, ".fdl") == 0) {
                snprintf(filePath, sizeof(filePath), "%s/%s", dirPath, entry->d_name);
                if (unlink(filePath)) {
                    perror("删除文件失败");
                } else {
                    printf("已删除: %s\n", filePath);
                }
            }
        }
    }
    closedir(dir);
}
