#ifndef PZFILESCLIENT_H
#define PZFILESCLIENT_H
#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef __linux__
#include <arpa/inet.h>
#include <sys/socket.h>
#endif
#ifdef __WIN32
#include <winsock2.h>
#include <ws2tcpip.h>   // Windows
#endif
#include "PzTypes.h"
extern PPCTask *task;
extern void setFDLTaskProgress(pz_uint8_t stat);
extern void setEVRTaskProgress(pz_uint8_t stat);
extern void setCliTrainNum(pz_uint8_t n);
extern pz_uint8_t getSrvTrainNum();
extern pz_uint8_t getCab1Stat();
extern pz_uint8_t getCab6Stat();
extern pz_uint16_t getTrainSpeed();
extern pz_uint8_t getManualCmd();
extern pz_uint8_t getCabNum();
extern int initCliTask(const char* remote_ip,
						handleSpeed hsfunc,
						handleManCmd hmfunc);
extern pz_int32_t startCliTask();
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
#endif // PZFILESCLIENT_H
