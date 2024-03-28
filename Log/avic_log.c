/**
* @file      avicLog.c
* @brief     日志管理源文件
*
* 函数定义
* @copyright http://www.avic-intl-sz.cn/
* @author    xh
* @version   1.0.0
* @date      2021-4-6
* @par history:
* | version | date | author | description |
* | ------- | ---- | ------ | ----------- |
* | 1.0.0 | 2021-4-6 | xh | create |
*/

//内部头文件
/* #include "log.h" */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/wait.h>
#include <sys/syscall.h>

//内部定义
#define LOG_FOLDER_NAME     "log/"              /* 日志目录 */
#define LOG_FILE_NAME       "log"               /* 文件名称 */
#define MAX_LOG_SIZE        (2 * 1024 * 1024)   /* 每个文件大小 */
#define MAX_LOG_NUM         5                   /* 文件个数 */
#define MAX_BUF_LEN         65535               /* 通用最大buf长度 */
#define MAX_TIME_LEN        32                  /* 时间长度 */
#define MAX_MESSAGE_NUMBER  100                 /* 最大消息个数 */
#define ENABLE_WRITE_FLAG   1                   /* 写文件标志 */
#define MAX_STRING_SIZE     4096                /* 日志最大字符长度 */
#define SOFTWARE_VERSION    "V1.0.2"            /* 软件版本 */
#define LOCAL_INET_ADDR     "127.0.0.1"         /* 本地内部地址，内部通信 */
#define LOG_UDP_PORT        9110                /* udp服务器发送端口 */

/*
* @brief 消息数据结构体
*/
typedef struct Message{
    char* mes;
    int   len;
}MessageData;

/*
* @brief 消息队列结构体
*/
typedef struct MessageNumber
{
    int write_flage;                                /* 1代表开始写，0代表停止写 */
    int index;
    MessageData message_data[MAX_MESSAGE_NUMBER];
}MessageNumberData;

static MessageNumberData mes_num_data;

/*
* @brief      获取日志文件夹路径
* @note
* @param[out] path    文件路径
* @param[in]  maxlen  最大路径长度
* @return     执行结果
* @retval     0       成功
* @retval     其他    失败
*/
static int GetLogFolderPath(char *path, int maxlen)
{
    char buf[MAX_BUF_LEN] = {0};
    int count = readlink("/proc/self/exe", buf, MAX_BUF_LEN);
    if ((count < 0) || (count >= (MAX_BUF_LEN - 1))) {
        return -1;
    } else {
        for (int i = (count - 1); i >= 0; --i) {
            if (buf[i] == '/') {
                buf[i + 1] = '\0';
                strcat(buf, LOG_FOLDER_NAME);
                break;
            }
        }
    }
    count = strlen(buf);
    if (count > maxlen) {
        return -1;
    }
    strcpy(path, buf);
    return 0;
}

/*
* @brief      获取系统时间字符串
* @note
* @param[out] buf     时间数据指针
* @param[in]  maxlen  数据最大长度
* @param[in]  off_time时区偏差
* @return     执行结果
* @retval     0       成功
* @retval     其他    失败
*/
int GetSysTimeString(char *buf, int max_len, int off_time)
{
    if ((buf == NULL) || (max_len <= 0) || (off_time < -11) || (off_time > 11)) {
        return -1;
    }

    struct tm *tm_now;
    struct timeval tv_now;

    gettimeofday(&tv_now, NULL);
    tv_now.tv_sec += (off_time * 3600);
    tm_now = gmtime(&tv_now.tv_sec);
    snprintf(buf, max_len, "%d-%02d-%02d %02d:%02d:%02d.%06d",
             tm_now->tm_year + 1900, tm_now->tm_mon + 1, tm_now->tm_mday,
             tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec, (int)tv_now.tv_usec);

    return 0;
}

/*
* @brief      初始化日志文件，没有日志目录则创建日志目录
* @note
* @param[out] path    路径
* @return     执行结果
* @retval     0       成功
* @retval     其他    失败
*/
static int InitLogFile(char *path)
{
    int ret = -1;
    ret = GetLogFolderPath(path, MAX_BUF_LEN);
    if (ret) {
        return -1;
    }
    ret = access(path, F_OK);
    if (ret == 0) {
        strcat(path, LOG_FILE_NAME);
        return 0;
    }
    ret = mkdir(path, 0777);
    if (ret == 0) {
        strcat(path, LOG_FILE_NAME);
        return 0;
    }
    return -1;
}

/*
* @brief      缓存日志
* @note
* @param[in]  buf      buf数据
* @param[in]  buf_size buf数据长度
* @retval     0       成功
* @retval     其他    失败
*/
static int LogBuffer(char* buf, int buf_size)
{
    int index;
    if ((buf == NULL) || (buf_size > MAX_STRING_SIZE) || (mes_num_data.write_flage == ENABLE_WRITE_FLAG)) 
    {
        return -1;
    }
    index = mes_num_data.index;

    if (index >= MAX_MESSAGE_NUMBER) {
        mes_num_data.index = 0;
        mes_num_data.write_flage = ENABLE_WRITE_FLAG;
        return 0;
    } 
    mes_num_data.message_data[index].mes = (char* )malloc(buf_size);
    memcpy(mes_num_data.message_data[index].mes, buf, buf_size);
    mes_num_data.message_data[index].len = buf_size;
    mes_num_data.index++;

    return 0;
}

/*
* @brief      日志文件命名
* @note
* @param[in]  log_fp     文件描述符
* @param[in]  file_size  文件大小
* @param[in]  path       文件路径
* @retval     无
*/
static void LogFileRename(FILE **log_fp, int file_size, char* path)
{
    int ret = 0;
    char tmp_buf1[MAX_BUF_LEN] = {0}, tmp_buf2[MAX_BUF_LEN] = {0};

    /* check log file size */
    if (file_size > MAX_LOG_SIZE) {
        /* close log_fp */
        fclose(*log_fp);
        for (int i = MAX_LOG_NUM; i > 0; i--) {
            memset(tmp_buf1, 0, MAX_BUF_LEN);
            sprintf(tmp_buf1, "%s%d", path, i);
            /* check log file exist */
            ret = access(tmp_buf1, F_OK);
            if (ret == 0) {
                if (i == MAX_LOG_NUM) {
                    /* remove oldest log file */
                    remove(tmp_buf1);
                } else {
                    /* rename log file */
                    memset(tmp_buf2, 0, MAX_BUF_LEN);
                    sprintf(tmp_buf2, "%s%d", path, (i + 1));
                    rename(tmp_buf1, tmp_buf2);
                }
            }
        }
        /* rename log file */
        memset(tmp_buf2, 0, MAX_BUF_LEN);
        sprintf(tmp_buf2, "%s%d", path, 1);
        rename(path, tmp_buf2);
        *log_fp = fopen(path, "a+");
        if (NULL == *log_fp) {
            printf("open log file err\n");
            return;
        }

    }
    return;
}

/*
* @brief      创建inet udp服务端socket
* @note
* @param[out] fd      socket句柄
* @param[in]  addr    本地地址
* @param[in]  port    本地端口
* @return     执行结果
* @retval     0       成功
* @retval     其他    失败
*/
int LogCreateInetDgramSocket(int* fd, const char* addr, unsigned short port)
{
    int ret = -1, tmp_fd = -1;

    tmp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (tmp_fd < 0) {
        return -1;
    }

    if (port > 0) {
        /* 添加addr */
        struct sockaddr_in sock_addr;
        memset(&sock_addr, 0, sizeof(struct sockaddr_in));
        sock_addr.sin_family = AF_INET;
        sock_addr.sin_port = htons(port);
        sock_addr.sin_addr.s_addr = inet_addr(addr);
        ret = bind(tmp_fd, (struct sockaddr *)&sock_addr, sizeof(struct sockaddr));
        if (ret < -1) {
            return ret;
        }
    }

    *fd = tmp_fd;
    return 0;
}

/*
* @brief                安全字符格式化函数
* @note       
* @param[in]  buf       格式化字符
* @param[in]  buf_len   最大长度
* @param[in]  fmt       可变参数
* @return     0         失败
* @return     其他      格式化长度
*/
int
LogSafeSprintf(char* buf, int max_len, char* fmt, ...)
{
    if (buf == NULL || max_len <= 0) {
        return 0;
    }

    va_list args;
    va_start(args, fmt);
    int ret = vsnprintf(buf, max_len, fmt, args);
    va_end(args);

    if (ret == -1) {
        buf[0] = '\0';
        return 0;
    }

    if (ret >= max_len) {
        buf[max_len - 1] = '\0';
    }

    return ret;
}

/*
* @brief      main函数
*
* @return     
* @retval            
* @retval         
*/
int main(int argc, char *argv[])
{
    int ret = -1, srv_fd = -1, file_size = 0, bytes_read = 0, tmp_len, max_len;
    char log_file_path[MAX_BUF_LEN] = {0}, tmp_buf1[MAX_BUF_LEN] = {0}, tmp_buf2[MAX_BUF_LEN] = {0}, time_buf[MAX_BUF_LEN] = {0};
    char log_write_buf[MAX_BUF_LEN] = {0}, recv_buf[MAX_BUF_LEN] = {0};
    
    /* 无视SIGPIPE信号，防止连接断开时产生SIGPIPE信号终止进程 */
    signal(SIGPIPE, SIG_IGN);
    
    /* 创建服务器本地SOCKET */
    ret = LogCreateInetDgramSocket(&srv_fd, LOCAL_INET_ADDR, LOG_UDP_PORT);
    if (ret) {
        printf("log create_server_unix_dgram_socket err\n");
        return -1;
    }
    
    /* 初始化日志文件名 */
    ret = InitLogFile(log_file_path);
    if (ret) {
        printf("open log folder err\n");
        return -1;
    }
    
    /* 打开日志文件 */
    FILE *log_fp = fopen(log_file_path, "a+");
    if (NULL == log_fp) {
        printf("open log file err\n");
        return -1;
    }
    
    /* 写入日志版本 */
    ret = fprintf(log_fp, "%s %s \n", "AVIC Log SoftWare Version", SOFTWARE_VERSION);
    fflush(log_fp);

    while (1) {
        memset(recv_buf, 0, MAX_BUF_LEN);
        bytes_read = recvfrom(srv_fd, recv_buf, sizeof(recv_buf), 0, NULL, NULL);
        if (bytes_read <= 0) {
            usleep(10);
            /* perror("recvfrom error\n"); */
            continue;
        }
        while (1) {
            tmp_len = 0, max_len = 0;
            tmp_len = strlen(recv_buf + max_len) + 1;
            memset(time_buf, 0, sizeof(time_buf));
            GetSysTimeString(time_buf, sizeof(time_buf), 8); /* 使用北京时间 */
            ret = LogSafeSprintf(log_write_buf, MAX_BUF_LEN, "[%s]:%s", time_buf, (recv_buf + max_len));
            if (!ret) {
                break;
            } else {
                log_write_buf[ret] = '\0';
            }
            LogBuffer(log_write_buf, ret + 1);

            if (mes_num_data.write_flage == ENABLE_WRITE_FLAG) {
                for (int index = 0; index < MAX_MESSAGE_NUMBER; index++) {
                    fprintf(log_fp, "%s\n", mes_num_data.message_data[index].mes);
                    free(mes_num_data.message_data[index].mes);
                    mes_num_data.message_data[index].mes = NULL;

                    fseek(log_fp, 0L, SEEK_END);
                    file_size = ftell(log_fp);
                    /* rename log file */
                    LogFileRename(&log_fp, file_size, log_file_path);
                }
                mes_num_data.write_flage = 0;
            } 
            
            max_len += tmp_len;
            if (max_len >= bytes_read) {
                break;
            }
        }
    }
    close(srv_fd);
    return 0;
}