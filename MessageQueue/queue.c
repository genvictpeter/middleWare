/**
* @file      queue.c
* @brief     消息队列源文件
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
#include "queue.h"
#define DEFAULT_SIZE    1024

/*
* @brief      环形队列创建
* @note       
* @param[in]  size    队列长度
* @return     队列指针
* @retval     无
*/
QueueData* QueueCreate(int size)
{
    QueueData* q = (QueueData* )malloc(sizeof(QueueData));
    if (q != NULL) {
        if (size > 0) {
            q->buf = malloc(size);
            q->capcity = size;
        } else {
            q->buf = malloc(DEFAULT_SIZE * sizeof(void *)); /* 8*1024 */
            q->capcity = DEFAULT_SIZE;
        }
        q->header = q->tail = q->size = 0;
    }
    return q;
}

/*
* @brief      环形队列是否充满
* @note       
* @param[in]  q    队列结构体
* @return     1    成功
* @return     0    失败
*/
int QueueIsFull(QueueData* q)
{
    return q->size == q->capcity;
}

/*
* @brief      环形队列是否为空
* @note       
* @param[in]  q    队列结构体
* @return     1    成功
* @return     0    失败
*/
int QueueIsEmpty(QueueData* q)
{
    return q->size == 0;
}

/*
* @brief      环形队列尾部插入元素
* @note       
* @param[in]  q    队列结构体
* @param[in]  data 数据
* @return     0    成功
* @return     -1   失败
*/
int QueuePushTail(QueueData* q, void* data)
{
    if (q == NULL) {
        return -1;
    }

    if (!QueueIsFull(q)) {
        q->buf[q->tail] = data;
        q->tail = (q->tail + 1) % q->capcity;
        q->size++;
    }
    return 0;
}

/*
* @brief      环形队列头部取出元素
* @note       
* @param[in]  q    队列结构体
* @return     void 数据指针
*/
void* QueuePopHead(QueueData* q)
{
    void* data = NULL;
    if (!QueueIsEmpty(q)) {
        data = q->buf[q->header];
        q->header = (q->header + 1) % q->capcity;
        q->size--;        
    }
   
    return data;
}

/*
* @brief      释放环形队列
* @note       
* @param[in]  q    队列结构体
* @return     0    成功
* @return     -1   失败
*/
int QueueFree(QueueData* q)
{
    if ((q == NULL) || (q->buf == NULL)) {
        return -1;
    }
    free(q->buf);
    q->buf = NULL;
    free(q);
    q = NULL;
    return 0;
}

/*
* @brief      条件变量实现异步队列
* @note       
* @param[in]  size 队列长度
* @return     队列指针
*/
AsyncQueueData* AsyncQueueDataCreate(int size)
{
    AsyncQueueData* mq = (AsyncQueueData* )malloc(sizeof(AsyncQueueData));
    mq->async_queue = QueueCreate(size);
    mq->wait_pthread = 0;
    pthread_mutex_init(&(mq->m_mutex), NULL);
    pthread_cond_init(&(mq->m_cond), NULL);

    return mq;
}


/*
* @brief      环形队列尾部插入元素
* @note       
* @param[in]  mq    队列结构体
* @param[in]  data 数据
* @return     0    成功
* @return     -1   失败
*/
int AsyncQueuePushTail(AsyncQueueData* mq, void* data)
{
    /* int ret = pthread_mutex_trylock(&(mq->m_mutex)); */

    if (mq == NULL) {
        return -1;
    }

    if (!QueueIsFull(mq->async_queue)) {
        QueuePushTail(mq->async_queue, data);
        if (mq->wait_pthread > 0) { 
            pthread_cond_signal(&(mq->m_cond));       
        }
    } 
    pthread_mutex_unlock(&(mq->m_mutex));
    usleep(1);
    return 0;
}

/*
* @brief      环形队列头部取出元素
* @note       
* @param[in]  q    队列结构体
* @param[in]  tv   时间参数
* @return     void 数据指针
*/
void* AsyncQueuePopHead(AsyncQueueData* mq, struct timeval* tv)
{
    /* int ret = pthread_mutex_trylock(&(mq->m_mutex)); */

    void *retval = NULL;
    /* 头部为空等待 */
    if (QueueIsEmpty(mq->async_queue)) {
        mq->wait_pthread++;
        while (QueueIsEmpty(mq->async_queue)) {
            pthread_cond_wait(&(mq->m_cond), &(mq->m_mutex));
        }
        mq->wait_pthread--;
    }
    retval = QueuePopHead(mq->async_queue);
    pthread_mutex_unlock(&(mq->m_mutex));
    usleep(1);
    return retval;
}

/*
* @brief      释放环形队列
* @note       
* @param[in]  q    队列结构体
* @return     0    成功
* @return     -1   失败
*/
int AsyncQueueFree(AsyncQueueData* mq)
{
    if (mq == NULL) {
        return -1;
    }
    QueueFree(mq->async_queue);
    pthread_mutex_destroy(&(mq->m_mutex));
    pthread_cond_destroy(&(mq->m_cond));
    free(mq);
    mq = NULL;
    return 0;
}