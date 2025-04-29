/*
 * Copyright (c) [2025] [Liu Shitong]
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
 * version:测试版:0.0.3
 * date:2025-4-29
 */
#include "global.h"
#include "readInfo.h"
#include "download.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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
static uint16_t crc16_false(unsigned char *puchMsg, unsigned int usDataLen);
static void *downloadFileFunc(void *arg);
static char lastTime[256] = "";
static u_int8_t fdlDownloadProgress = 0x55;
static DownloadThreadArgs DlThreadArgs = {false,false};
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
    initCliTask("192.168.0.5",warningSpeed,recvManCmd);//server ip
    printf("init cli task finished..\n");
    startCliTask();
    
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
        char command[256];
        //sprintf(command, "ping -c 1 192.168.0.1 > /dev/null");
        sprintf(command, "ping -c 1 %s > /dev/null",model->vcuIp);
        int ping_result = system(command);
        if(ping_result == 0){
            consecutive_pings++;
            if (consecutive_pings >= CONSECUTIVE_PINGS 
                && !DlThreadArgs.thread_started 
                && isUpTime(model->hour,model->min,model->sec) == true
                && getTrainSpeed() == 0){
                dzlog_debug("第%d次下载开始",tryTotal);
                pthread_create(&thread_id, NULL, downloadFileFunc, (void *)&DlThreadArgs);
                pthread_join(thread_id, NULL);
                if(DlThreadArgs.threadResult == true){
                    consecutive_pings = 0;
                    DlThreadArgs.threadResult = false;
                    dzlog_debug("下载成功");
                    tryTotal = 1;
                    sleep(model->sleepTime);
                }else if(DlThreadArgs.threadResult == false){
                    dzlog_debug("第%d次下载失败",tryTotal);
                    tryTotal++;
                }
                if(tryTotal > MAX_RETRY){
                    dzlog_debug("当天下载失败");
                    consecutive_pings = 0;
                    tryTotal = 1;
                    sleep(model->sleepTime);
                }
            }
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

static void *downloadFileFunc(void *arg) {
    DownloadThreadArgs *args = (DownloadThreadArgs *)arg;
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
    sockfd = connectServer(model->vcuIp,model->flashId);
    if(sockfd <= 0){
        longjmp(jump_buffer,RI_CONNECT_SERVER_ERR);
    }
    sendReadInfo();
    close(sockfd);

    unsigned int max_first_timestamp = htonl((carBlocks)->first_timestamp);
    unsigned int min_first_timestamp = htonl((carBlocks)->first_timestamp);
    int thread_function_i = 0;
    for(thread_function_i = 0;thread_function_i < carBlocksAmount - 1;thread_function_i++){
        if(max_first_timestamp < htonl((carBlocks + thread_function_i + 1)->first_timestamp)){
            max_first_timestamp = htonl((carBlocks + thread_function_i + 1)->first_timestamp);
        }
        if(min_first_timestamp > htonl((carBlocks + thread_function_i + 1)->first_timestamp)){
            min_first_timestamp = htonl((carBlocks + thread_function_i + 1)->first_timestamp);
        }
    }
    //printf("%u %u \n",max_first_timestamp,min_first_timestamp);
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
    sendDownloadStatus(fdlFileName);
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

// static void *udpThread(void *arg){
//     int sockfd;
//     struct sockaddr_in local_addr, remote_addr;
//     socklen_t addr_len = sizeof(remote_addr);
//     char recvBuffer[RECV_BUFFER_SIZE];

//     sockfd = socket(AF_INET, SOCK_DGRAM, 0);
//     if (sockfd < 0) {
//         perror("socket creation failed");
//         exit(EXIT_FAILURE);
//     }

//     // 绑定本地接收端口
//     memset(&local_addr, 0, sizeof(local_addr));
//     local_addr.sin_family = AF_INET;
//     local_addr.sin_addr.s_addr = INADDR_ANY;
//     local_addr.sin_port = htons(LOCAL_PORT);

//     if (bind(sockfd, (const struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
//         perror("bind failed");
//         close(sockfd);
//         exit(EXIT_FAILURE);
//     }

//     // 设置远程发送地址
//     memset(&remote_addr, 0, sizeof(remote_addr));
//     remote_addr.sin_family = AF_INET;
//     remote_addr.sin_port = htons(REMOTE_PORT);
//     inet_pton(AF_INET, REMOTE_IP, &remote_addr.sin_addr);

//     // 设置非阻塞模式
//     fcntl(sockfd, F_SETFL, O_NONBLOCK);

//     printf("UDP Send/Receive started on port %d...\n", LOCAL_PORT);

//     while (true) {
//         // 发送数据
//         unsigned char sendBuffer[SEND_BUFFER_SIZE] = {0};  // UDP 数据包

//         // 填充协议数据
//         sendBuffer[0] = 0xFA;    // 协议开始符
//         sendBuffer[1] = 0x01;    // 功能码
//         sendBuffer[2] = fdlDownloadProgress;    // FDL下载进度
//         sendBuffer[3] = 0x00;    // EVR下载进度
//         sendBuffer[4] = 0x01;    // 列车号
//         sendBuffer[5] = 0x00;    // 保留字段

//         // 计算 CRC-16 校验
//         uint16_t crc = crc16_false(sendBuffer, 6);
//         sendBuffer[6] = (crc >> 8) & 0xFF;  // 高字节
//         sendBuffer[7] = crc & 0xFF;         // 低字节

//         sendBuffer[8] = 0xFB;    // 协议结束符

//         sendto(sockfd, sendBuffer, strlen(sendBuffer), 0, (struct sockaddr *)&remote_addr, addr_len);
//         printf("Sent: %s\n", sendBuffer);

//         // 接收数据（非阻塞）
//         memset(recvBuffer, 0, RECV_BUFFER_SIZE);
//         ssize_t recv_len = recvfrom(sockfd, recvBuffer, RECV_BUFFER_SIZE, 0, 
//                                     (struct sockaddr *)&remote_addr, &addr_len);
//         if (recv_len != RECV_BUFFER_SIZE) {
//             printf("Invalid packet size: %zd\n", recv_len);
//             continue;
//         }

//         // 解析数据包
//         if (recvBuffer[0] != 0xFA || recvBuffer[30] != 0xFB) {
//             printf("Invalid packet format!\n");
//             continue;
//         }

//         uint16_t received_crc = (recvBuffer[28] << 8) | recvBuffer[29];
//         uint16_t calculated_crc = crc16_false(recvBuffer, 28);

//         printf("Received Packet from %s:%d\n", inet_ntoa(remote_addr.sin_addr), ntohs(remote_addr.sin_port));
//         printf("功能码: 0x%02X\n", recvBuffer[1]);
//         printf("数据序号: %u\n", (recvBuffer[2] << 8) | recvBuffer[3]);
//         printf("数据长度: %u\n", (recvBuffer[4] << 8) | recvBuffer[5]);
//         printf("司机室1激活: %u\n", recvBuffer[6]);
//         printf("司机室6激活: %u\n", recvBuffer[7]);
//         printf("速度: %u km/h\n", (recvBuffer[8] << 8) | recvBuffer[9]);
//         printf("手动下载指令: %u\n", recvBuffer[10]);
//         printf("列车号: %u\n", recvBuffer[11]);

//         if (received_crc == calculated_crc) {
//             printf("CRC 校验成功\n");
//         } else {
//             printf("CRC 校验失败！(Received: 0x%04X, Expected: 0x%04X)\n", received_crc, calculated_crc);
//         }

//         printf("-----------------------------------\n");

//         // 周期500ms
//         usleep(500);
//     }

//     close(sockfd);
// }

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