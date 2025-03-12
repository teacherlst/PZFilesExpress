/*
 * Copyright (c) [2024] [Liu Shitong]
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
 */
#include "global.h"
#include "readInfo.h"
#include "download.h"
#define MYRFIFO "/opt/project/NJL5_DDU/ddu_to_fdl_fifo"
#define MYWFIFO "/opt/project/NJL5_DDU/fdl_to_ddu_fifo"
extern char *optarg;
extern int optind, opterr, optopt;
static void readMessageFromConfigFile(char *srcFile,char *srcStr,char *tarStr);
static int directory_exists(const char *path);
static void *thread_function(void *arg);
static void *personDownload_function(void *arg);
static char fdlFileName[256] = "";
static char lastTime[256] = "";
static PingThreadArgs pingThreadArgs;
static pthread_mutex_t mutex; 

#define CONSECUTIVE_PINGS 5
int main(int argc,char *argv[]){
    int zlogId = dzlog_init("./zlog.conf", "FDL");
    if (zlogId) {
        printf("init failed, please check file %s.\n", "zlog.conf");
        exit(EXIT_FAILURE);
    }
    readCarConfigFile(CARCONFIGFILENAME);
    mkfifo(MYRFIFO, 0776); // 创建命名管道
    mkfifo(MYWFIFO, 0776); // 创建命名管道
    char currentTime[256];
    //printf("sleeptime = %d\n",sleepTime);
    int consecutive_pings = 0;
    int thread_started = 0;
    int tryTotal = 1;
    pingThreadArgs.threadResult = false;
    pingThreadArgs.initDownload = false;
    pthread_t thread_id;
    pthread_t personDownload;
    pthread_t getYearMonthDayThread;
    pthread_create(&personDownload, NULL, personDownload_function, NULL);

    while (true) {
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        sprintf(currentTime,"%04d/%02d/%02d-%02d:%02d:%02d",
            tm.tm_year+1900,tm.tm_mon+1,tm.tm_mday,
            tm.tm_hour,tm.tm_min,tm.tm_sec);
        char command[100];
        sprintf(command, "ping -c 1 %s > /dev/null",carIp);
        int ping_result = system(command);
        if (ping_result == 0) {
            consecutive_pings++;
            if (consecutive_pings >= CONSECUTIVE_PINGS && !thread_started && ddu_to_fdl.speed == 0) {
                if(tryTotal <= 3){
                    //时间戳
                    int isUp16_30 = isUpTime(hour,min,sec);
                    if(isUp16_30 == true){
                        int fd = open(MYWFIFO, O_WRONLY); // 打开管道以写入
                        if (fd == -1) {
                            dzlog_debug("自动下载管道打开失败");
                            sleep(sleepTime);
                            continue;
                        }
                        fdl_to_ddu.command = FDL_TO_DDU_START;
                        int a = write(fd, &fdl_to_ddu, sizeof(FDL_TO_DDU)); // 写入 bool 值
                        printf("写入:%d\n",a);
                        printf("向DDU发送开始下载\n");
                        close(fd);
                        // 开启线程执行函数
                        if (pthread_create(&thread_id, NULL, thread_function, (void *)&pingThreadArgs)){
                            dzlog_debug("自动下载进程创建失败");
                            sleep(sleepTime);
                            continue;
                        }
                        printf("Thread started.\n");
                        thread_started = 1;
                        tryTotal++;
                    }
                }else{
                    dzlog_fatal("当天传输失败");
                    pingThreadArgs.threadResult = false;
                    consecutive_pings = 0;
                    tryTotal = 1;
                    sleep(sleepTime);
                }
            }
        }else {
            consecutive_pings = 0;
            if (thread_started) {
                // 终止线程
                pthread_cancel(thread_id);
                printf("Thread cancelled.\n");
                thread_started = 0;
            }
            printf("failure\n");
        }
        if(pingThreadArgs.threadResult == true){
            pingThreadArgs.threadResult = false;
            consecutive_pings = 0;
            thread_started = 0;
            tryTotal = 1;
            int fd = open(MYWFIFO, O_WRONLY); // 打开管道以写入
            fdl_to_ddu.command = FDL_TO_DDU_END;
            write(fd, &fdl_to_ddu, sizeof(FDL_TO_DDU)); // 写入 bool 值
            close(fd);
            sleep(sleepTime);
        }
        //printf("%d \n",pingThreadArgs.threadResult);
        sleep(1);
    }
    zlog_fini();
    unlink(MYRFIFO); // 删除命名管道
    unlink(MYWFIFO); // 删除命名管道
    return 0;
}
static int directory_exists(const char *path) {
    struct stat info;
    if (stat(path, &info) != 0) {
        return 0; 
    }
    return S_ISDIR(info.st_mode);
}




static void readMessageFromConfigFile(char *srcFile,char *srcStr,char *tarStr){
    int offset = offsetFile(srcFile,srcStr);
    FILE *configFile = fopen(srcFile,"r");
    if(configFile == NULL){
        printf("打开config.ini配置文件失败\n");
    }
    fseek(configFile,offset,SEEK_SET);
    fscanf(configFile,"%s\n",tarStr);
    fclose(configFile);
}


static void *thread_function(void *arg) {
    PingThreadArgs *args = (PingThreadArgs *)arg;
    char timeStr[256];
    char blockMaxTime[256];
    char blockMinTime[256];
    char timeRange[512];
    // 在这里执行你想要执行的函数
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    sprintf(timeStr,"%04d-%02d-%02d_%02d-%02d-%02d",
        tm.tm_year+1900,tm.tm_mon+1,tm.tm_mday,
        tm.tm_hour,tm.tm_min,tm.tm_sec);


    dzlog_debug("-----------------------------%04d-%02d-%02d start------------------------------",tm.tm_year+1900,tm.tm_mon+1,tm.tm_mday);

    int* ret = malloc(sizeof(int));

    char log_Path[50] = "./log";
    if (!directory_exists(log_Path)) {
        if (mkdir(log_Path, 0777) == -1) {
            dzlog_debug("创建日志文件夹失败");
        }
    }

    bool ret_sendReadInfo = true;
    ret_sendReadInfo = sendReadInfo();
    
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
    //printf("%s %s \n",blockMaxTime,blockMinTime);
    if(args->initDownload == false){
        strcpy(lastTime,blockMinTime);
        args->initDownload = true;
    }else{
        readStringFromINI(CARCONFIGFILENAME,"Settings","lastTime",lastTime,sizeof(lastTime));
    }
    writeStringToIni(CARCONFIGFILENAME,"Settings","lastTime",lastTime);
    sprintf(timeRange,"%s%s%s",lastTime,",",blockMaxTime);
    sprintf(fdlFileName,"%s/%s(%s).fdl",log_Path,timeRange,timeStr);
    //printf("%s\n",timeRange);

    getTime_s_e(timeRange);
    int dr_rs = sendDownloadRequest(fdlFileName);
    if(dr_rs == false){
        dzlog_fatal("当天传输无新数据块");
        dzlog_debug("-----------------------------%s-%s-%s end------------------------------",year,month,day);
        args->initDownload = true;
        args->threadResult = true;
        return NULL;
    }else if(dr_rs == FTPSOCKET_OPEN_FAILURE){
        dzlog_fatal("连接远端服务器失败");
        dzlog_debug("-----------------------------%s-%s-%s end------------------------------",year,month,day);
        args->initDownload = true;
        args->threadResult = true;
        return NULL;
    }
    printf("%s\n",fdlFileName);
    sendDownloadStatus(fdlFileName);
    char cdCmd[256];
    sprintf(cdCmd,"%s%s%s","cd ",fdlSaveDir,year);
    int cd_rs = ftp_Connect(cdCmd);
    if(cd_rs == false){
        char mkDirCmd[256];
        sprintf(mkDirCmd,"%s%s%s","mkdir ",fdlSaveDir,year);
        ftp_Connect(mkDirCmd);
    }

    sprintf(cdCmd,"%s%s%s/%s","cd ",fdlSaveDir,year,month);
    cd_rs = ftp_Connect(cdCmd);
    if(cd_rs == false){
        char mkDirCmd[256];
        sprintf(mkDirCmd,"%s%s%s/%s","mkdir ",fdlSaveDir,year,month);
        ftp_Connect(mkDirCmd);
    }

    char upLoadCmd[256];
    sprintf(upLoadCmd,"%s%s%s%s%s/%s/%s(%s)%s","upload ",fdlFileName," ",fdlSaveDir,year,month,timeRange,timeStr,".fdl");
    args->threadResult = ftp_Connect(upLoadCmd);
    if(carBlocks != NULL){
        free(carBlocks);
    }

    pthread_exit(ret);
    dzlog_debug("当天传输成功");
    dzlog_debug("-----------------------------%s-%s-%s end------------------------------",year,month,day);
}

static void *personDownload_function(void *arg) {
    while (true)
    {
        int read_fd = open(MYRFIFO, O_RDONLY); // 打开管道以读取
        read(read_fd, &ddu_to_fdl, sizeof(DDU_TO_FDL)); // 读取 bool 值
        if(ddu_to_fdl.command == DDU_TO_FDL_START){
            ddu_to_fdl.command = DO_NOTHING;
            close(read_fd);
            int write_fd = open(MYWFIFO, O_WRONLY); // 打开管道以写入
            fdl_to_ddu.command = FDL_TO_DDU_START; // 设置 bool 值
            int a = write(write_fd, &fdl_to_ddu, sizeof(FDL_TO_DDU)); // 写入 bool 值
            pthread_t tmp_id;
            pthread_create(&tmp_id, NULL, thread_function, (void *)&pingThreadArgs);
            pthread_join(tmp_id,NULL);
            if(pingThreadArgs.threadResult == true){
                pingThreadArgs.threadResult = false;
                fdl_to_ddu.command = FDL_TO_DDU_END;
                int a = write(write_fd, &fdl_to_ddu, sizeof(FDL_TO_DDU)); // 写入 bool 值
                close(write_fd);
                dzlog_debug("-----------------------------%s-%s-%s 人为传输成功------------------------------",year,month,day);
            }
        }
    }
}