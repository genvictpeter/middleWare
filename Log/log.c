/*
* @file      log.c
* @brief     日志源文件
*
* 函数定义
* @copyright http://www.avic-intl-sz.cn/
* @author    xh
* @version   1.0.0
* @date      2021-4-7
* @par history:
* | version | date | author | description |
* | ------- | ---- | ------ | ----------- |
* | 1.0.0 | 2021-4-7 | xh | create |
*/  

#include "log.h"

static LogFolter log_ft;

/*
* @brief      日志初始化
* @note       配置日志等级、颜色、关键字、标签
* @param[in]  level    日志等级
* @return     执行结果
* @retval     无
*/
void 
LogInit(int level, const char* keyword, const char* tag)
{
    log_ft.level = level;
    //log_ft.enable_color = color;
    if (log_ft.level < 0) {
        log_ft.level = 0;  
    }

    if (keyword == NULL) {
        if (tag == NULL) {
            return;
        } else {
            strncpy(log_ft.tag, tag, sizeof(log_ft.tag));
            log_ft.tag[MAX_FILTER_NAME_LEN] = '\0';
        }
        return;
    } else {
        strncpy(log_ft.keyword, keyword, sizeof(log_ft.keyword));
        log_ft.keyword[MAX_FILTER_NAME_LEN] = '\0';
    }
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
int 
LogCreateInetDgramSocket(int* fd, const char* addr, unsigned short port)
{
    int ret = -1, tmp_fd = -1;

    tmp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (tmp_fd < 0) {
        return -1;
    }
    
/*     socklen_t argsize = sizeof(rcvsize);
    rcvsize = 5 * 1024 * 1024;
    ret = setsockopt(tmp_fd, SOL_SOCKET, SO_RCVBUF, (void* )&rcvsize, argsize);
    if (ret < 0) {
        printf("getsockopt error");
        close(tmp_fd);
        return -1;
    } */

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
* @brief      inet udp socket数据发送
* @note
* @param[in]  fd      socket句柄
* @param[in]  addr    目的地址
* @param[in]  port    目的端口
* @param[in]  msg     发送数据
* @param[in]  len     发送数据长度
* @return     执行结果  
* @retval     0       成功
* @retval     其他    失败
*/
int 
LogSendInetDgramSocket(int fd, const char *addr, unsigned short port, const char *msg, int len)
{
    struct sockaddr_in sock_addr;
    memset(&sock_addr, 0, sizeof(struct sockaddr_in));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(port);
    sock_addr.sin_addr.s_addr = inet_addr(addr);
    return sendto(fd, msg, len, 0, (struct sockaddr *)&sock_addr, sizeof(sock_addr));
}


/*
* @brief      格式化字符串(可变参数)
* @note
* @param[in]  size    字符串缓存最大长度
* @param[in]  format  字符串格式
* @param[out] str     格式化的字符串
* @return     字符串长度
* @retval     <0      执行失败
* @retval     其他    格式化的字符串长度.
*/
int 
LogFormatString(char *str, int size, const char *format, ...)
{
    if ((str == NULL) || (format == NULL)) {
        return -1;
    }
    va_list ap;
    va_start(ap, format);
    vsnprintf(str, size, format, ap);
    va_end(ap);
    return 0;
}

/*
* @brief      打印日志(可变参数)
* @note
* @param[in]  file    代码文件名
* @param[in]  line    代码行数
* @param[in]  level   日志级别
* @param[in]  log_id  模块ID
* @param[in]  format  格式化的字符串
* @return     无
*/
void 
Log(const char* file, const char* func, int line, LogLevelEnum level, const char* log_id, int flag, const char* format, ...)
{
    if ((level < log_ft.level) || (ENABLE_OUTPUT_CMD_FILE < flag)) {
        return;  
    } 
    
    /* 标签过滤 */
    if (log_ft.tag[0] != '\0') {
        if (strcmp(log_id, log_ft.tag)) {
            return;
        }
    }

    int ret = -1;
    static int s_fd = -1;
    if (s_fd <= 0) {
        ret = LogCreateInetDgramSocket(&s_fd, NULL, 0);
        if (ret != 0) {
            s_fd = -1;
            return;
        }
    }
    char buf[MAX_COM_BUF_LEN] = {0}, out_buf[MAX_COM_BUF_LEN] = {0};

    va_list vp;
    va_start(vp, format);
    vsnprintf(buf, MAX_COM_BUF_LEN, format, vp);
    va_end(vp);
    
    char array_level[LEVEL_MAX]={'V', 'D', 'I', 'W', 'E', 'A'};
    if (level >= LEVEL_MAX) {
        return;
    }
    /* 日志等级格式化 */
    snprintf(out_buf, MAX_COM_BUF_LEN, "[%c][PID:%d TID:%ld][%s %s][LINE:%d]%s->:%s", array_level[level], getpid(), syscall(SYS_gettid), file, func, line, log_id, buf);

    /* 关键字过滤 */
    if (log_ft.keyword[0] != '\0') {
        if (!strstr(out_buf, log_ft.keyword)) {
            return;
        }
    }

    /* 日志实时开关控制 */
    char *array_level_color[LEVEL_MAX] = {PRT_VERBOSE, PRT_DEBUG, PRT_INFO, PRT_WARNING, PRT_ERROR, PRT_ASSERT};
    if (flag > QUIT) {
        if (flag != ONLY_WRITE) {
            PRINTF_COLOR(array_level_color[level], out_buf);
        } 
        if (flag > ONLY_READ) {
            ret = LogSendInetDgramSocket(s_fd, LOCAL_INET_ADDR, LOG_UDP_PORT, out_buf, (strlen(out_buf) + 1));
            if (ret <= 0) {
                perror("send log error");
            }
        }
    }

    return;
}

/*
* @brief      打印自定义字符数组
* @note
* @param[in]  head_buf    打印数据头
* @param[in]  buf         字符数组缓存
* @param[in]  len         字符数组长度
* @param[in]  flag        打印标识
* @param[in]  log_id      日志ID
* @param[in]  level       日志级别
* @return     无
*/
void 
LogBuf(const char* head_buf, const char* buf, int len, LogLevelEnum level, const char* log_id, int flag)
{
    char out_buf[MAX_COM_BUF_LEN] = {0};
    snprintf(out_buf, MAX_COM_BUF_LEN, "%s:%d:", head_buf, len);
    int tmp_len = strlen(out_buf) + len * 3 + 1;
    if (tmp_len >= MAX_COM_BUF_LEN) {
        LOG(WARNING, log_id, flag, "LOG_BUF len err");
        return;
    }
    tmp_len = strlen(out_buf);
    for (int i = 0; i < len; i++) {
        sprintf(out_buf + tmp_len, "%02x ", buf[i]);
        tmp_len += 3;
    }
    LOG(level, log_id, flag, "%s", out_buf);

    return;
} 
