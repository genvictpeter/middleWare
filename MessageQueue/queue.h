/**
* @file      queue.h
* @brief     消息队列头文件
*
* 
* @copyright http://www.avic-intl-sz.cn/
* @author    xh
* @version   1.0.0
* @date      2021-4-22
* @par history:
* | version | date | author | description |
* | ------- | ---- | ------ | ----------- |
* | 1.0.0 | 2021-4-22 | xh | create |
*/
#ifndef __LOCK_H__INCLUDE_
#define __LOCK_H__INCLUDE_

#pragma pack(1)

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*
* @brief      队列数据结构体
*/
typedef struct Queue{
    int     header;             /* 头部 */                        
    int     tail;               /* 尾部 */
    int     size;               /* 队列长度 */
    int     capcity;            /* 容量 */
    void**  buf;                /* 数据 */
}QueueData;

/*
* @brief      异步队列数据结构体
*/
typedef struct AsyncQueue{
    pthread_mutex_t     m_mutex;                /* 线程互斥锁 */
    pthread_cond_t      m_cond;                 /* 线程条件变量 */
    int                 wait_pthread;           /* 等待线程数量 */
    QueueData*          async_queue;            /* 队列数据 */
}AsyncQueueData;

/*
* @brief      环形队列创建
* @note       
* @param[in]  size    队列长度
* @return     队列指针
* @retval     无
*/
QueueData* QueueCreate(int );

/*
* @brief      环形队列是否充满
* @note       
* @param[in]  q    队列结构体
* @return     1    成功
* @return     0    失败
*/
int QueueIsFull(QueueData* );

/*
* @brief      环形队列是否为空
* @note       
* @param[in]  q    队列结构体
* @return     1    成功
* @return     0    失败
*/
int QueueIsEmpty(QueueData* );

/*
* @brief      环形队列尾部插入元素
* @note       
* @param[in]  q    队列结构体
* @param[in]  data 数据
* @return     0    成功
* @return     -1   失败
*/
int QueuePushTail(QueueData* , void* );

/*
* @brief      环形队列头部取出元素
* @note       
* @param[in]  q    队列结构体
* @return     void 数据指针
*/
void* QueuePopHead(QueueData* );

/*
* @brief      释放环形队列
* @note       
* @param[in]  q    队列结构体
* @return     0    成功
* @return     -1   失败
*/
int QueueFree(QueueData* );

/*
* @brief      条件变量实现异步队列
* @note       
* @param[in]  size 队列长度
* @return     队列指针
*/
AsyncQueueData* AsyncQueueDataCreate(int );

/*
* @brief      环形队列尾部插入元素
* @note       
* @param[in]  mq    队列结构体
* @param[in]  data 数据
* @return     0    成功
* @return     -1   失败
*/
int AsyncQueuePushTail(AsyncQueueData* , void* );

/*
* @brief      环形队列头部取出元素
* @note       
* @param[in]  q    队列结构体
* @param[in]  tv   时间参数
* @return     void 数据指针
*/
void* AsyncQueuePopHead(AsyncQueueData* , struct timeval* );

/*
* @brief      释放环形队列
* @note       
* @param[in]  q    队列结构体
* @return     0    成功
* @return     -1   失败
*/
int AsyncQueueFree(AsyncQueueData* );

#endif