#include "readInfo.h"
extern void sendReadInfo(){
    if(access(MESSAGEFILENAME,F_OK) == 0){
        remove(MESSAGEFILENAME);
    }
    TFDFrameHead frameHead = {"FDHEADS",
                             htonl(READINFO),
                              "FDHEADE"};

    TFDReadInfoFrame readInfoFrame = {"FDBODYS",
                                    htonl(model->flashId),
                                      "FDBODYE"};
    ssize_t ret_FrameHead = send(sockfd,&frameHead,sizeof(TFDFrameHead),0);
    ssize_t ret_ReadInfoFrame = send(sockfd,&readInfoFrame,sizeof(TFDReadInfoFrame),0);
    if(0 > ret_FrameHead || 0 > ret_ReadInfoFrame){
        longjmp(jump_buffer, RI_SEND_ERR);
    }
    dzlog_debug("*********************************sendReadInfo 向VCU服务端发送[%ld]字节数据*********************************",ret_FrameHead + ret_ReadInfoFrame);

    ssize_t totalTypes = 0;
    ssize_t ret_Types = recv(sockfd,&socketBuffer,sizeof(socketBuffer),0);
    while (ret_Types != 0)
    {
        totalTypes += ret_Types;
        ret_Types = recv(sockfd,&socketBuffer[totalTypes],sizeof(socketBuffer)-totalTypes,0);
    }
    dzlog_debug("*********************************sendReadInfo 接收VCU服务器[%ld]字节数据*********************************",totalTypes);

    TFDFrameHead ret_TFDFrameHead_Struct;
    memcpy(&ret_TFDFrameHead_Struct,socketBuffer,sizeof(char)*20);
    if(strcmp(ret_TFDFrameHead_Struct.startTag,"FDHEADS") != 0){
        longjmp(jump_buffer, RI_FRAME_HEAD_ERR);
    }

    TFDReadInfoFrame ret_TFDReadInfoFrame_Struct;
    memcpy(ret_TFDReadInfoFrame_Struct.startTag,socketBuffer + 20,sizeof(char)*8);
    if(strcmp(ret_TFDReadInfoFrame_Struct.startTag,"FDBODYS") != 0){
        longjmp(jump_buffer, RI_DATA_HEAD_ERR);
    }

    fl04_id_info_r pidInfo;
    memcpy(&pidInfo,socketBuffer + 28,sizeof(char)*20);
    dzlog_info("用户在执行sendReadInfo函数时，从服务器读取到的块信息:\n"
                "NAND FLASH INFO:\n"
                "Total amount:%d\n"
                "Current Block:%d\n"
                "current chip:%d\n"
                "current page:%d\n"
                "data waiting:%d\n"
                ,htonl(pidInfo.amount),htonl(pidInfo.current_block),htonl(pidInfo.current_chip)
                ,htonl(pidInfo.current_page),htonl(pidInfo.data_waiting));

    fl04_id_info_page blockinfo;
    char timestr[256];
    unsigned int cnt=htonl(pidInfo.amount)+10;
    if(cnt == 10){
        longjmp(jump_buffer, RI_NODATA_ERR);
    }

    unsigned int timetamp_2000_1_1_00_00_00 = 946656000;
    int ret_Total_Block = 0;
    int tmp_total = 0;
    while (cnt > 10)
    {
        memset(&blockinfo,0,sizeof(char)*28);
        memcpy(&blockinfo,socketBuffer + 48 + (htonl(pidInfo.amount)+10 - cnt)*28,sizeof(char)*28);
        time_t ftstamp = htonl(blockinfo.first_timestamp);

        if(ftstamp > timetamp_2000_1_1_00_00_00){
            ret_Total_Block += 1;
            struct tm *local_time = localtime(&ftstamp);
            sprintf(timestr,"%04d-%02d-%02d %02d:%02d:%02d\n",
                local_time->tm_year+1900,local_time->tm_mon+1,local_time->tm_mday,
                local_time->tm_hour,local_time->tm_min,local_time->tm_sec);
            // dzlog_debug("用户在执行sendReadInfo函数时，从服务器读取到的信息:\n"
            //     "%d\tpage_used:%u\t\tpage_read:%d\t\tchip:%d\t\tblock:%d\t\tcurrent_counter:%d\t\terase_counter:%d\t\ttime:%s\n",
            //     ret_Total_Block,
            //     htonl(blockinfo.pages_read),
            //     htonl(blockinfo.pages_used),
            //     htonl(blockinfo.chip),
            //     htonl(blockinfo.block),
            //     htonl(blockinfo.current_counter),
            //     htonl(blockinfo.erase_counter),
            //                     timestr);
            local_time = NULL;
            memcpy(totalBlocks + tmp_total*28,&blockinfo,sizeof(fl04_id_info_page));
            tmp_total += 1;
        }
        cnt--;
    }
    carBlocksAmount = tmp_total;
    carBlocks = malloc(sizeof(fl04_id_info_page)*tmp_total);
    fl04_id_info_page blockinfo_2;
    int sendReadInfo_i = 0;
    for (sendReadInfo_i = 0; sendReadInfo_i < carBlocksAmount; sendReadInfo_i++) {
        memset(&blockinfo_2, 0, sizeof(fl04_id_info_page));
        memcpy(&blockinfo_2, totalBlocks + sendReadInfo_i * sizeof(fl04_id_info_page), sizeof(fl04_id_info_page));
        memcpy(carBlocks + sendReadInfo_i,&blockinfo_2,sizeof(fl04_id_info_page));
    }
    writeBlkMsgToFile(totalBlocks,tmp_total);
    dzlog_debug("成功从服务器读取块信息写入信息文件");
    memcpy(ret_TFDReadInfoFrame_Struct.endTag,socketBuffer + totalTypes-8,sizeof(char)*8);
    if(strcmp(ret_TFDReadInfoFrame_Struct.endTag,"FDBODYE") != 0){
        longjmp(jump_buffer, RI_DATA_TAIL_ERR);
    }
}

static void writeBlkMsgToFile(char *downloadBlocks_p,int totalBlocks_p){
    FILE *blockMessagefile;
    // 文件路径
    // 打开文件，如果文件不存在则创建，如果存在则覆盖
    blockMessagefile = fopen(MESSAGEFILENAME, "w");
    if (blockMessagefile == NULL) {
        fopenFileErr(MESSAGEFILENAME);
    }
    fprintf(blockMessagefile,"blockAmount=%d\n",totalBlocks_p);
    fl04_id_info_page blockinfo;
    int writeBlockMessageToFile_i = 0;
    for (writeBlockMessageToFile_i = 0; writeBlockMessageToFile_i < totalBlocks_p; writeBlockMessageToFile_i++) {
        memset(&blockinfo, 0, sizeof(fl04_id_info_page));
        memcpy(&blockinfo, downloadBlocks_p + writeBlockMessageToFile_i * sizeof(fl04_id_info_page), sizeof(fl04_id_info_page));
        size_t writeSize = fwrite(&blockinfo, sizeof(fl04_id_info_page), 1, blockMessagefile);
        fprintf(blockMessagefile,"\n");
        if (writeSize != 1) {
            fwriteFileErr(MESSAGEFILENAME);
        }
    }
    fclose(blockMessagefile);
}