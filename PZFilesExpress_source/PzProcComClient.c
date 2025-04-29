#include "PzProcComClient.h"

#define BUFFER_SIZE 1024    // 缓冲区大小
PPCTask *task = NULL;

void setFDLTaskProgress( pz_uint8_t stat)
{
    if(task->task_initOk != 1) return;
#ifdef __WIN32
    WaitForSingleObject(task->send_task->mutex, INFINITE);
#endif
#ifdef __linux__
    pthread_mutex_lock(&task->send_task->mutex);
#endif
    task->send_task->pack->fdl_task_progress = stat;
#ifdef __linux__
    pthread_mutex_unlock(&task->send_task->mutex);
#endif
};

void setEVRTaskProgress( pz_uint8_t stat)
{
    if(task->task_initOk != 1) return;
#ifdef __WIN32
    WaitForSingleObject(task->send_task->mutex, INFINITE);
#endif
#ifdef __linux__
    pthread_mutex_lock(&task->send_task->mutex);
#endif
    task->send_task->pack->evr_task_progress = stat;
#ifdef __linux__
    pthread_mutex_unlock(&task->send_task->mutex);
#endif
};

void setCliTrainNum(pz_uint8_t n)
{
    if(task->task_initOk != 1) return;
#ifdef __WIN32
    WaitForSingleObject(task->send_task->mutex, INFINITE);
#endif
#ifdef __linux__
    pthread_mutex_lock(&task->send_task->mutex);
#endif
    task->send_task->pack->train_num = n;
#ifdef __linux__
    pthread_mutex_unlock(&task->send_task->mutex);
#endif
}

pz_uint8_t getSrvTrainNum()
{
    if(task->task_initOk != 1) return 0;
#ifdef __WIN32
    WaitForSingleObject(task->recv_task->mutex, INFINITE);
#endif
#ifdef __linux__
    pthread_mutex_lock(&task->recv_task->mutex);
#endif
    pz_uint8_t _tn = task->recv_task->pack->train_num;
#ifdef __linux__
    pthread_mutex_unlock(&task->recv_task->mutex);
#endif
    return _tn;
}

pz_uint8_t getCab1Stat()
{
    if(task->task_initOk != 1) return 0;
#ifdef __WIN32
    WaitForSingleObject(task->recv_task->mutex, INFINITE);
#endif
#ifdef __linux__
    pthread_mutex_lock(&task->recv_task->mutex);
#endif
    pz_uint8_t _stat = task->recv_task->pack->cab1_stat;
#ifdef __linux__
    pthread_mutex_unlock(&task->recv_task->mutex);
#endif
    return _stat;
}

pz_uint8_t getCab6Stat()
{
    if(task->task_initOk != 1) return 0;
#ifdef __WIN32
    WaitForSingleObject(task->recv_task->mutex, INFINITE);
#endif
#ifdef __linux__
    pthread_mutex_lock(&task->recv_task->mutex);
#endif
    pz_uint8_t _stat = task->recv_task->pack->cab6_stat;
#ifdef __linux__
    pthread_mutex_unlock(&task->recv_task->mutex);
#endif
    return _stat;
}

pz_uint8_t getManualCmd()
{
    if(task->task_initOk != 1) return 0;
#ifdef __WIN32
    WaitForSingleObject(task->recv_task->mutex, INFINITE);
#endif
#ifdef __linux__
    pthread_mutex_lock(&task->recv_task->mutex);
#endif
    pz_uint8_t _cmd = task->recv_task->pack->cab1_stat;
#ifdef __linux__
    pthread_mutex_unlock(&task->recv_task->mutex);
#endif
    return _cmd;
}

pz_uint16_t getTrainSpeed()
{
    if(task->task_initOk != 1) return 0;
#ifdef __WIN32
    WaitForSingleObject(task->recv_task->mutex, INFINITE);
#endif
#ifdef __linux__
    pthread_mutex_lock(&task->recv_task->mutex);
#endif
    pz_uint8_t _speed = ntohs(task->recv_task->pack->speed);
#ifdef __linux__
    pthread_mutex_unlock(&task->recv_task->mutex);
#endif
    return _speed;
}

int initCliTask(const char* remote_ip,
				handleSpeed hsfunc,
				handleManCmd hmfunc)
{
    task = NULL;
    task = (PPCTask*)malloc(sizeof(PPCTask));
    memset(task,0,sizeof(PPCTask));
    //recv task
    task->recv_task = (DsToExTask*)malloc(sizeof(DsToExTask));
#ifdef __linux__
    task->recv_task->pid = 0;
#endif
    task->recv_task->pack = (DsToExPack*)malloc(sizeof(DsToExPack));
    memset(task->recv_task->pack,0,sizeof(DsToExPack));
    task->recv_task->pack->speed = 0xFFFF;
	//callback for speed and mancmd
	if(hsfunc != NULL)
		task->pSpeedCallBack = hsfunc;
	if(hmfunc != NULL)
		task->pManCmdCallBack = hmfunc;
    //send pack
    task->send_task = (ExToDsTask*)malloc(sizeof(ExToDsTask));
#ifdef __linux__
    task->send_task->pid = 0;
#endif
    task->send_task->pack = (ExToDsPack*)malloc(sizeof(ExToDsPack));
    task->send_task->pack->start_char = 0xFA;
    task->send_task->pack->fuc_code = 0x01;
    memset(&task->send_task->pack->fdl_task_progress,0,6);
    strcpy(task->remote_ip,remote_ip);

    if( task == NULL||
        task->recv_task == NULL||
        task->recv_task->pack == NULL||
        task->send_task == NULL||
        task->send_task->pack == NULL)
    {
        printf("client task init failed..\n");
        return 0;
    }
    printf("client task init OK..\n");
    task->task_initOk = 1;
    return 1;
}

#ifdef __WIN32
//子线程向PzFilesDataSource发送数据
static DWORD WINAPI SendThread_Win(LPVOID lpParam)
{
    PPCTask *ptask = (PPCTask*)lpParam;
    WSADATA wsaData;
    struct sockaddr_in server_addr;
    // 初始化Winsock库
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed.\n");
        return 1;
    }
    // 创建UDP套接字
    if ((ptask->send_task->sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET) {
        printf("create remote socket error：%d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // 初始化服务器地址结构
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;                 // IPv4
    server_addr.sin_port = htons(PZFILEDATASOURCE_PORT);               // 服务器端口
    if (inet_pton(AF_INET, ptask->remote_ip, &server_addr.sin_addr) <= 0) {
        printf("invalid ip \n");
        closesocket(ptask->send_task->sockfd);
        WSACleanup();
        return 1;
    }

    while(1)
    {
        WaitForSingleObject(ptask->send_task->mutex, INFINITE);
        if (sendto(ptask->send_task->sockfd, (char*)ptask->send_task->pack, sizeof(ExToDsPack), 0,
                   (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            printf("send message error：%d\n", WSAGetLastError());
            return 1;
        }
        Sleep(1000);
    }

    closesocket(ptask->send_task->sockfd);
    WSACleanup();
    return 0;
}

//子线程收PzFilesDataSource数据
static DWORD WINAPI RecvThread_Win(LPVOID lpParam)
{
    PPCTask *ptask = (PPCTask*)lpParam;
    char buffer[BUFFER_SIZE];
    WSADATA wsaData;
    struct sockaddr_in server_addr, client_addr;

    // 初始化Winsock库
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed.\n");
        return 1;
    }

    // 创建UDP套接字
    if ((ptask->recv_task->sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET) {
        printf("create local socket error：%d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // 初始化服务器地址结构
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PZFILESEXPRESS_PORT);

    // 绑定套接字到指定地址和端口
    if (bind(ptask->recv_task->sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("bind error：%d\n", WSAGetLastError());
        closesocket(ptask->recv_task->sockfd);
        WSACleanup();
        return 1;
    }

    printf("UDP listening %d...\n", PZFILESEXPRESS_PORT);

    socklen_t addr_len = sizeof(client_addr); // 客户端地址结构大小

    while (1) {
        // 接收客户端发送的数据
        int n = recvfrom(ptask->recv_task->sockfd, buffer, BUFFER_SIZE, 0,
                        (struct sockaddr*)&client_addr, &addr_len);
        if (n == SOCKET_ERROR) {
            printf("recv message error：%d\n", WSAGetLastError());
            continue;
        }
        if(n != sizeof(DsToExPack))
        {
            printf("recv message length error\n");
            continue;
        }
        WaitForSingleObject(task->recv_task->mutex, INFINITE);
        memcpy(&task->recv_task->pack,buffer, sizeof(DsToExPack));
        printf("client recv start %d\n",ptask->recv_task->pack->start_char);
    }
    return 0;
}
#endif

#ifdef __linux__
static void *SendThread(void *arg)
{
    printf("client send thread started ..\n");
    PPCTask *ptask = (PPCTask*)arg;
    struct sockaddr_in server_addr;

    // 创建UDP套接字
    if ((ptask->send_task->sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("创建套接字失败");
        exit(EXIT_FAILURE);
    }

    // 初始化服务器地址结构
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PZFILEDATASOURCE_PORT);
    if (inet_pton(AF_INET, ptask->remote_ip, &server_addr.sin_addr) <= 0) {
        printf("无效的服务器IP地址:%s\n",ptask->remote_ip);
        close(ptask->send_task->sockfd);
        exit(EXIT_FAILURE);
    }

    while(1)
    {
        pthread_mutex_lock(&ptask->send_task->mutex);
        int sendcnt = sendto(ptask->send_task->sockfd,
                            (char*)ptask->send_task->pack, sizeof(ExToDsPack),
                             0, (const struct sockaddr *)&server_addr, sizeof(server_addr));
        pthread_mutex_unlock(&ptask->send_task->mutex);
        if (sendcnt < 0) {
            perror("发送消息失败");
            close(ptask->send_task->sockfd);
            exit(EXIT_FAILURE);
        }
        sleep(1);
    }
    // 关闭套接字
    close(ptask->send_task->sockfd);
}

static void *RecvThread(void *arg)
{
    printf("client recv thread started ..\n");
    PPCTask *ptask = (PPCTask*)arg;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in server_addr, client_addr;

    // 创建UDP套接字
    if ((task->recv_task->sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("create local socket error");
        exit(EXIT_FAILURE);
    }

    // 初始化服务器地址结构
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PZFILESEXPRESS_PORT);

    if (bind(task->recv_task->sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind local socket error");
        close(task->recv_task->sockfd);
        exit(EXIT_FAILURE);
    }

    printf("UDP listening %d...\n", PZFILESEXPRESS_PORT);

    socklen_t addr_len = sizeof(client_addr);

    while (1) {
        int n = recvfrom(task->recv_task->sockfd, buffer, BUFFER_SIZE, 0,
                        (struct sockaddr*)&client_addr, &addr_len);
        if (n == SO_ERROR) {
            perror("recv message error..\n");
            continue;
        }
        if(n != sizeof(DsToExPack))
        {
            printf("recv message length error\n");
            continue;
        }
        printf("recv message length:%d \n",n);
        pthread_mutex_lock(&ptask->recv_task->mutex);
        memcpy(ptask->recv_task->pack,buffer, sizeof(DsToExPack));
        pthread_mutex_unlock(&ptask->recv_task->mutex);
        //printf("Recv from Server:\ncab1:%d\ncab6:%d\nspeed:%d\nmancmd:%d\ntrainnum:%d\n",
        //       task->recv_task->pack->cab1_stat,
        //       task->recv_task->pack->cab6_stat,
        //       ntohs(task->recv_task->pack->speed),
        //       task->recv_task->pack->manual_cmd,
        //       task->recv_task->pack->train_num);
		if(task->pSpeedCallBack != NULL)
			task->pSpeedCallBack(ntohs(task->recv_task->pack->speed));
		if(task->pManCmdCallBack != NULL)
			task->pManCmdCallBack(task->recv_task->pack->manual_cmd);
    }

    close(task->recv_task->sockfd);
}
#endif

pz_int32_t startCliTask()
{
    if(task->task_initOk != 1)
        return 1;
#ifdef __WIN32
    // 创建互斥量
    task->send_task->mutex = CreateMutex(NULL, FALSE, NULL);
    if (task->send_task->mutex == NULL) {
        printf("create send_mutex error:%lu\n", GetLastError());
        return 2;
    }

    task->recv_task->mutex = CreateMutex(NULL, FALSE, NULL);
    if (task->recv_task->mutex == NULL) {
        printf("create recv_mutex error:%lu\n", GetLastError());
        return 3;
    }

    task->send_task->hThread = CreateThread(NULL,0,SendThread_Win,task,0,NULL);
    task->recv_task->hThread = CreateThread(NULL,0,RecvThread_Win,task,0,NULL);

    WaitForSingleObject(task->send_task->hThread, INFINITE);
    CloseHandle(task->send_task->hThread);
    WaitForSingleObject(task->recv_task->hThread, INFINITE);
    CloseHandle(task->recv_task->hThread);
#endif
#ifdef __linux__
    // 创建互斥量
    pthread_mutex_init(&task->send_task->mutex,NULL);
    pthread_mutex_init(&task->recv_task->mutex,NULL);

    pthread_create(&task->send_task->pid, NULL, SendThread, (void *)task);
    pthread_create(&task->recv_task->pid, NULL, RecvThread, (void *)task);
#endif
    return 0;
}

void waitCliTask()
{
    pthread_join(task->send_task->pid,NULL);
    pthread_join(task->recv_task->pid,NULL);
}

static int stopCliSendTask()
{
#ifdef __linux__
    int res = 0;
    void* rval = NULL;
    res = pthread_cancel(task->send_task->pid);
    if(res != 0 )
    {
        printf("stop Cli send thread failed...\n");
        return res;
    }
    res = pthread_join(task->send_task->pid,&rval);

    if(rval == PTHREAD_CANCELED)
        printf("Cli send thread stoped...\n");
    else
        return 1;
    pthread_mutex_destroy(&task->send_task->mutex);
    close(task->send_task->sockfd);
#endif
    return 0;
}

static int stopCliRecvTask()
{
#ifdef __linux__
    int res = 0;
    void* rval = NULL;
    res = pthread_cancel(task->recv_task->pid);
    if(res != 0 )
    {
        printf("stop Cli recv thread failed...\n");
        return res;
    }
    res = pthread_join(task->recv_task->pid,&rval);

    if(rval == PTHREAD_CANCELED)
        printf("Cli recv thread stoped...\n");
    else
        return 1;
    pthread_mutex_destroy(&task->recv_task->mutex);
    close(task->recv_task->sockfd);
#endif
    return 0;
}

pz_int32_t stopCliTask()
{
    if(stopCliSendTask() == 0 &&
       stopCliRecvTask() == 0)
    {
        printf("Cli task stoped..\n");
        return 0;
    }
    return 1;
}
