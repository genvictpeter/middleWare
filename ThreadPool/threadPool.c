/**
* @file      threadPool.c
* @brief     线程池源文件
*
* 函数定义
* @copyright http://www.avic-intl-sz.cn/
* @author    xh
* @version   1.0.0
* @date      2021-5-20
* @par history:
* | version | date | author | description |
* | ------- | ---- | ------ | ----------- |
* | 1.0.0 | 2021-5-20 | xh | create |
*/

#include "threadPool.h"

/**
* @brief      工作线程
* @note  							
* @param[in]  threadpool            线程池指针
* @return     无                   		
*/
static void *ThreadPoolWork(void *threadpool)
{
    ThreadPool *pool = (ThreadPool *)threadpool;
    ThreadPoolTask task;

    for (;;) {
        pthread_mutex_lock(&(pool->lock));

        /* 阻塞 */
        while ((pool->count == 0) && (!pool->shutdown)) {
            pthread_cond_wait(&(pool->cond), &(pool->lock));
        }

        if ((pool->shutdown == ImmediateShutDown) ||
           ((pool->shutdown == GracefulShutDown) &&
            (pool->count == 0))) {
            break;
        }

        /* 加载任务 */
        task.func = pool->queue[pool->head].func;
        task.arg = pool->queue[pool->head].arg;
        pool->head = (pool->head + 1) % pool->queue_size;
        pool->count -= 1;

        pthread_mutex_unlock(&(pool->lock));

        /* 执行任务 */
        (*(task.func))(task.arg);
        printf("%d pool->count = %d \n", __LINE__, pool->count);
    }

    pool->start_thread_number--;

    pthread_mutex_unlock(&(pool->lock));
    pthread_exit(NULL);
    return(NULL);
}

/**
* @brief      释放线程池
* @note  							
* @param[in]  pool              线程池指针
* @return     0                 成功 
* @return     其他              失败                   		
*/
static int ThreadPoolFree(ThreadPool *pool)
{
    if (pool == NULL || pool->start_thread_number > 0) {
        return -1;
    }

    /* 释放分配内存 */
    if (pool->threads) {
        free(pool->threads);
        free(pool->queue);
        pool->threads = NULL;
        pool->queue = NULL;

        pthread_mutex_lock(&(pool->lock));
        pthread_mutex_destroy(&(pool->lock));
        pthread_cond_destroy(&(pool->cond));
    }
    free(pool); 
    pool = NULL;   
    return 0;
}

/**
* @brief      创建线程池
* @note  							
* @param[in]  thread_number      线程数量
* @param[in]  queue_size        队列范围
* @return     无   		
*/
ThreadPool *ThreadPoolCreate(int thread_number, int queue_size)
{
    ThreadPool *pool;
    int i;
    /* (void) flags; */

    if (thread_number <= 0 || thread_number > MAX_THREADS || queue_size <= 0 || queue_size > MAX_QUEUE) {
        return NULL;
    }

    if ((pool = (ThreadPool *)malloc(sizeof(ThreadPool))) == NULL) {
        goto err;
    }

    /* 初始化 */
    pool->thread_number = 0;
    pool->queue_size = queue_size;
    pool->head = pool->tail = pool->count = 0;
    pool->shutdown = pool->start_thread_number = 0;

    /* 分配内存 */
    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_number);
    pool->queue = (ThreadPoolTask *)malloc(sizeof(ThreadPoolTask) * queue_size);

    /* 初始化mutex和cond */
    if ((pthread_mutex_init(&(pool->lock), NULL) != 0) ||
       (pthread_cond_init(&(pool->cond), NULL) != 0) ||
       (pool->threads == NULL) ||
       (pool->queue == NULL)) {
        goto err;
    }

    /* 创建工作线程 */
    for (i = 0; i < thread_number; i++) {
        if(pthread_create(&(pool->threads[i]), NULL,ThreadPoolWork, (void*)pool) != 0) {
            ThreadPoolDestroy(pool, 0);
            return NULL;
        }
        pool->thread_number++;
        pool->start_thread_number++;
    }

    return pool;

 err:
    if (pool) {
        ThreadPoolFree(pool);
    }
    return NULL;
}


/**
* @brief      向线程池添加任务
* @note  							
* @param[in]  pool              线程池指针
* @param[in]  func              任务函数
* @param[in]  arg               函数参数
* @param[in]  flags             标志位
* @return     0                 成功   
* @return     其他              失败 		
*/
int ThreadPoolAppend(ThreadPool *pool, void (*func)(void *), void *arg)
{
    int err = 0;
    int next;
    /* (void) flags; */

    if (pool == NULL || func == NULL) {
        return ThreadPoolInvalid;
    }

    if (pthread_mutex_lock(&(pool->lock)) != 0) {
        return ThreadPoolLockFailure;
    }

    next = (pool->tail + 1) % pool->queue_size;

    do {
        /* 队列是否满 */
        if (pool->count >= pool->queue_size) {
            printf("pool queue is full\n");
            err = ThreadPoolQueueFull;
            break;
        }

        /* 判断是否关闭 */
        if (pool->shutdown) {
            err = ThreadPoolShutDown;
            break;
        }

        /* 增加到队列 */
        pool->queue[pool->tail].func = func;
        pool->queue[pool->tail].arg = arg;
        pool->tail = next;
        pool->count += 1;

        /* 唤醒工作线程 */
        if (pthread_cond_signal(&(pool->cond)) != 0) {
            err = ThreadPoolLockFailure;
            break;
        }
    } while(0);

    if (pthread_mutex_unlock(&pool->lock) != 0) {
        err = ThreadPoolLockFailure;
    }

    return err;
}

/**
* @brief      销毁线程池
* @note  							
* @param[in]  pool              线程池指针
* @param[in]  flags             关闭标志
* @return     0                 成功 
* @return     其他              失败                   		
*/
int ThreadPoolDestroy(ThreadPool *pool, int flags)
{
    int i, err = 0;

    if (pool == NULL) {
        return ThreadPoolInvalid;
    }

    if (pthread_mutex_lock(&(pool->lock)) != 0) {
        return ThreadPoolLockFailure;
    }

    do {

        if (pool->shutdown) {
            err = ThreadPoolShutDown;
            break;
        }

        /* 关闭标志判断 */
        pool->shutdown = (flags & GracefulShutDown) ? GracefulShutDown : ImmediateShutDown;

        /* 唤醒所有线程 */
        if ((pthread_cond_broadcast(&(pool->cond)) != 0) ||
           (pthread_mutex_unlock(&(pool->lock)) != 0)) {
            err = ThreadPoolLockFailure;
            break;
        }

        /* join所有线程 */
        for (i = 0; i < pool->thread_number; i++) {
            if (pthread_join(pool->threads[i], NULL) != 0) {
                err = ThreadPoolThreadFailure;
            }
        }
    } while(0);

    if (!err) {
        ThreadPoolFree(pool);
    }
    return err;
}





