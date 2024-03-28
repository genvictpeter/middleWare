/**
* @file      threadPool.h
* @brief     线程池头文件
*
* 包含通用的宏、枚举和结构体等类型定义
* @copyright http://www.avic-intl-sz.cn/
* @author    xh
* @version   1.0.0
* @date      2021-5-20
* @par history:
* | version | date | author | description |
* | ------- | ---- | ------ | ----------- |
* | 1.0.0 | 2021-5-20 | xh | create |
*/

#ifndef __THREADPOOL_H_  
#define __THREADPOOL_H_

#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_THREADS     64          /* 最大线程数 */
#define MAX_QUEUE       65536       /* 最大队列长度 */

/**
* @brief           线程池枚举
*/
typedef enum {
    ImmediateShutDown        = 1,   /* 立即关闭 */     
    GracefulShutDown         = 2,   /* 优雅关闭,处理已加载的任务 */     
    ThreadPoolInvalid        = -1,  /* 线程池无效 */          
    ThreadPoolLockFailure    = -2,  /* 线程池lock失败 */
    ThreadPoolQueueFull      = -3,  /* 线程池队列满 */
    ThreadPoolShutDown       = -4,  /* 线程池停止 */
    ThreadPoolThreadFailure  = -5   /* 线程池线程出错 */
} ThreadPoolEnum;

/**
* @brief           线程池任务结构体
*/
typedef struct {
    void (*func)(void *);           /* 任务函数 */
    void *arg;                      /* 传递功能参数 */
} ThreadPoolTask;

/**
* @brief            线程池结构体
*/
typedef struct ThreadPoolData {
  pthread_mutex_t lock;             /* 线程互斥锁 */
  pthread_cond_t cond;              /* 线程条件变量 */
  pthread_t *threads;               /* 总线程 */  
  ThreadPoolTask *queue;            /* 任务队列 */
  int thread_number;                /* 线程数 */
  int queue_size;                   /* 任务队列大小 */
  int head;                         /* 头 */
  int tail;                         /* 尾 */
  int count;                        /* 待执行任务数 */
  int shutdown;                     /* 关闭状态，不接受新任务，阻塞队列保存信息 */
  int start_thread_number;          /* 开始线程数 */
} ThreadPool;

/**
* @brief      创建线程池
* @note  							
* @param[in]  thread_number     线程数量
* @param[in]  queue_size        队列大小
* @return     ThreadPool   		线程池指针
*/
ThreadPool*
ThreadPoolCreate(int thread_number, int queue_size);

/**
* @brief      添加任务到线程池
* @note  							
* @param[in]  pool              线程池指针
* @param[in]  func              任务函数
* @param[in]  arg               函数参数
* @return     0                 成功   		
* @return     其他              失败
*/
int 
ThreadPoolAppend(ThreadPool *pool, void (*func)(void *), void *arg);

/**
* @brief      销毁线程池
* @note  							
* @param[in]  pool              线程池指针
* @param[in]  flags             标志
* @return     0                 成功 
* @return     其他              失败                   		
*/
int 
ThreadPoolDestroy(ThreadPool *pool, int flags);

#ifdef __cplusplus
}
#endif

#endif /* _THREADPOOL_H_ */
