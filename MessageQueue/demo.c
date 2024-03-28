/**
* @file      queue.h
* @brief     队列头文件
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



void* producer(void* data)
{
    AsyncQueueData* queue = (AsyncQueueData* )data;
    for (int i = 0; i < 10; i++) {
        AsyncQueuePushTail(queue, (void *)(&i));
        printf("push:%d \n", i);
        
    }
    return NULL;
}

void* consumer(void* data)
{
    AsyncQueueData* queue = (AsyncQueueData* )data;
    void* v_data = NULL;
    int* temp = NULL;
    for (int i = 0; i < 10; i++) {
        v_data = AsyncQueuePopHead(queue, NULL);
        if (v_data != NULL) {
            temp = (int* )v_data;  
            printf("pop:%d \n", *temp);
        }

    }
    return NULL;
}


int main(int argc, char *argv[])
{
    AsyncQueueData* queue = AsyncQueueDataCreate(4096);
    pthread_t pid[2];

    printf("void * is len = %ld \n", sizeof(void*));
    pthread_create(&pid[0], NULL, producer, (void *)queue);
    pthread_create(&pid[1], NULL, consumer, (void *)queue);

    pthread_join(pid[0], NULL);
    pthread_join(pid[1], NULL);

/*     for (int i = 0; i < 10; i++) {
        AsyncQueuePushTail(queue, (void* )&i);
        AsyncQueuePopHead(queue, NULL);
    }
 */
    AsyncQueueFree(queue);
    pthread_exit(NULL);
    return 0;
}