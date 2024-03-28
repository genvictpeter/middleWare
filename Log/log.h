/*
* @file      log.h
* @brief     日志头文件
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

#ifndef __SOCKET_H__INCLUDE_
#define __SOCKET_H__INCLUDE_

#ifdef __cplusplus 
extern "C"
{
#endif

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


/* #pragma once 
#pragma pack(1) */

    //---- 枚举开始 ----//

/*
* @brief 日志级别枚举 参考Android Logcat
*/
typedef enum {
    VERBOSE,       /* 详细 */
    DEBUG,         /* 调试 */
    INFO,          /* 描述 */
    WARNING,       /* 警告 */
    ERROR,         /* 错误 */
    ASSERT,        /* 断言 */
    LEVEL_MAX,      
}LogLevelEnum;

    //---- 枚举结束 ----//

    //---- 宏定义开始 ----//
#define SOFTWARE_VERSION          "V1.0.0"        /* 软件版本 */

/* 模块通信相关 */
#define LOG_UDP_PORT              9110            /* udp服务器发送端口 */
#define MAX_COM_BUF_LEN           65535           /* 通用buf长度 */
#define MAX_FILTER_NAME_LEN       15              /* 最大过滤长度 */
#define LOCAL_INET_ADDR           "127.0.0.1"     /* 本地内部地址，内部通信 */
#define INET_RX_ADDR              "0.0.0.0"       /* 本地所有IP地址 */

/* 日志flag控制 */
#define DISABLE_OUTPUT 				0               
#define ENABLE_OUTPUT_CMD  		  	1               
#define ENABLE_OUTPUT_FILE  	    2   
#define ENABLE_OUTPUT_CMD_FILE      3         

#define QUIT         DISABLE_OUTPUT                    /* 关闭日志打印 */
#define ONLY_READ    ENABLE_OUTPUT_CMD                 /* 只在命令行打印 */
#define ONLY_WRITE   ENABLE_OUTPUT_FILE                /* 只写入文件 */
#define READ_WRITE   ENABLE_OUTPUT_CMD_FILE            /* 写入文件和打印在命令行 */


/* 字体颜色 color */
#define F_NULL                         ""
#define F_BLACK                        "30m"
#define F_RED                          "31m"
#define F_GREEN                        "32m"
#define F_YELLOW                       "33m"
#define F_BLUE                         "34m"
#define F_MAGENTA                      "35m"
#define F_CYAN                         "36m"
#define F_WHITE                        "37m"

/* 字体背景 background */
#define B_NULL                         ""
#define B_BLACK                        "40;"
#define B_RED                          "41;"
#define B_GREEN                        "42;"
#define B_YELLOW                       "43;"
#define B_BLUE                         "44;"
#define B_MAGENTA                      "45;"                      /* 品红色 */
#define B_CYAN                         "46;"                      /* 蓝绿色 */
#define B_WHITE                        "47;"

/* 字体风格 style */
#define S_BOLD                         "1;"                       /* 加粗 */
#define S_UNDERLINE                    "4;"                       /* 加下画线 */
#define S_BLINK                        "5;"                       /* 闪烁 */
#define S_NORMAL                       "22;"                      /* 正常 */

#define START                          "\033["      
#define END                            "\033[0m"
#define PRT_VERBOSE                    (S_BOLD B_NULL F_BLUE)     /* 蓝色粗体 */ 
#define PRT_DEBUG                      (S_BOLD B_NULL F_GREEN)    /* 绿色粗体 */
#define PRT_INFO                       (S_BOLD B_NULL F_CYAN)     /* 蓝绿色粗体 */
#define PRT_WARNING                    (S_BOLD B_NULL F_YELLOW)   /* 黄色粗体 */ 
#define PRT_ERROR                      (S_BOLD B_NULL F_RED)      /* 绿色粗体 */
#define PRT_ASSERT                     (S_BOLD B_NULL F_MAGENTA)  /* 品红色粗体 */        

#define PRINTF_COLOR(prt_color, string)       printf("%s%s%s%s\n", START, prt_color, string, END)

#define LOG(level, id, flag, fmt, args...)    Log((const char* )__FILE__, (const char* )__FUNCTION__, __LINE__, level, id, flag, fmt, ##args)

#define LOG_BUF LogBuf


    /* ---- 宏定义结束 ---- */

    /* ---- 结构体定义开始 ---- */
/*
* @brief 日志配置结构体
*/
typedef struct {
  //size_t enable_color;                                       /* 等于1使能颜色打印，其他取消颜色打印 */
  size_t level;                                                /* 日志等级 */
  char   keyword[MAX_FILTER_NAME_LEN + 1];                     /* 关键字 */
  char   tag[MAX_FILTER_NAME_LEN + 1];                         /* 标签 */
}LogFolter;

    /* ---- 结构体定义结束 ---- */

    /* ---- 函数声明开始 ---- */

/*
  * @brief      日志等级
  * @note       小于等级level的日志信息将被过滤,默认为0
  * @param[in]  level    日志等级
  * @return     执行结果
  * @retval     无
  */
void LogInit(int , const char* ,const char* );                                                        

/*
  * @brief      udp服务端socket
  * @note       创建inet udp服务端socket
  * @param[out] fd      socket句柄
  * @param[in]  addr    本地地址
  * @param[in]  port    本地端口
  * @return     执行结果
  * @retval     0       成功
  * @retval     其他    失败
  */
int LogCreateInetDgramSocket(int* , const char* , unsigned short );                                       

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
int LogFormatString(char* , int , const char* , ...);                                                          

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
void Log(const char* , const char* , int , LogLevelEnum , const char* , int , const char* , ...);       

/*
* @brief      打印自定义字符数组
* @note 
* @param[in]  head_buf    打印数据头
* @param[in]  buf         字符数组缓存
* @param[in]  len         字符数组长度
* @param[in]  flag        打印标识
* @param[in]  log_id      日志ID
* @param[in]  level   日志级别
* @return     无
*/
void LogBuf(const char* , const char* , int , LogLevelEnum , const char* , int );                               


    /* ---- 函数声明结束 ---- */


#ifdef __cplusplus
}
#endif

#endif