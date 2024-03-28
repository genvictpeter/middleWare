/* #include "log.h" */
#include "memPool.h"
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/types.h> 
#include <pthread.h> 
#include <assert.h> 
#include <string.h>


#define NUM_THREADS 2
#define ID "MemPool"
#define NumberData 1000

#pragma pack(8)

int malloc_conut = 0;

struct thread_data{
   int  thread_id;
   char* message;
};

struct eDataNode{
    char* e_data;
};

struct DataNode{
    int count;
    char *list;
    struct eDataNode* pdata_node;
};

struct Data{
    int size;
    char name[16];
    struct DataNode* data_node;
};

char* list = "this is list,but test memory pool \n";
char* e_data = "end of zhe list,check data is true \n";
char timebuf[32] = {0};
char timebu_1[32] = {0};

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
static void DisplayPool(const mempool_alloc* pAprPool);

void testLog(void)
{
    int ret = -1;
    struct Data* mpdata[NumberData] = {NULL};
    GetSysTimeString(timebu_1,sizeof(timebu_1), 8);
    printf("testlog mpdata malloc before = %s \n", timebu_1);
	
    for (int i =0; i < NumberData; i++) {
        malloc_conut++;
        MemPoolAlloc(mpdata[i], sizeof(struct Data));
        /* mpdata[i] = (struct Data* )MemPoolAlloc(sizeof(struct Data)); */
        if (mpdata[i] == NULL) {
            printf("get pdata false");
        }

        mpdata[i]->size = sizeof(struct Data);
        memcpy(mpdata[i]->name, "Data", sizeof("Data"));

        /* 产生128-65536的随机数 */
        int rand_number_1 = rand() % (65536-128+1) + 128;
        MemPoolAlloc(mpdata[i]->data_node, sizeof(struct DataNode));
        /* mpdata[i]->data_node = (struct DataNode* )MemPoolAlloc(sizeof(struct DataNode)); */
        if (mpdata[i]->data_node == NULL) {
            printf("get pdata->data_node false");
			return;
        }
        mpdata[i]->data_node->count = malloc_conut;

        /* pdata->data_node->list */
        MemPoolAlloc(mpdata[i]->data_node->list, rand_number_1);
        /* mpdata[i]->data_node->list = (char* )MemPoolAlloc(rand_number_1); */
        if (mpdata[i]->data_node->list == NULL) {
            printf("get pdata->data_node->list false");
			return;
        }

        /* mpdata[i]->data_node->list = (void* )"this is list,but test memory pool \n"; */
        if (rand_number_1 > strlen(list)) {
            memcpy(mpdata[i]->data_node->list, list, strlen(list));
        }


        /* pdata->data_node->pdata_node */
        MemPoolAlloc(mpdata[i]->data_node->pdata_node, sizeof(struct eDataNode));
        /* mpdata[i]->data_node->pdata_node = (struct eDataNode* )MemPoolAlloc(sizeof(struct eDataNode)); */
        if (mpdata[i]->data_node->pdata_node == NULL) {
            printf("get mpdata[i]->data_node->pdata_node false"); 
			return;
        }


        /* pdata->data_node->pdata_node->e_data */
        MemPoolAlloc(mpdata[i]->data_node->pdata_node->e_data, sizeof(rand_number_1));
        /* mpdata[i]->data_node->pdata_node->e_data = (char* )MemPoolAlloc(rand_number_1); */
        if (mpdata[i]->data_node->pdata_node->e_data == NULL) {
            printf("get mpdata[i]->data_node->pdata_node false"); 
			return;
        }

        /* mpdata[i]->data_node->pdata_node->e_data = (void* )"end of zhe list,check data is true \n"; */
        if (rand_number_1 > strlen(e_data)) {
            memcpy(mpdata[i]->data_node->pdata_node->e_data, e_data, strlen(e_data)); 
        }

    }
    GetSysTimeString(timebu_1,sizeof(timebu_1), 8);
    
    for (int j = 0; j < NumberData; j++) {
		MemPoolFree(ret, mpdata[j]->data_node->pdata_node->e_data);
		MemPoolFree(ret, mpdata[j]->data_node->pdata_node);
		MemPoolFree(ret, mpdata[j]->data_node->list);
		MemPoolFree(ret, mpdata[j]->data_node);
		MemPoolFree(ret, mpdata[j]);
    }
    printf("testlog mpdata malloc later = %s \n", timebu_1);
    MemPoolSelect(ret);

    return;
}

void testLog1(void)
{

    struct Data* mpdata[NumberData] = {NULL};
    GetSysTimeString(timebu_1,sizeof(timebu_1), 8);
    printf("testlog1 mpdata malloc before = %s \n", timebu_1);

    for (int i =0; i < NumberData; i++) {
        malloc_conut++;        
        mpdata[i] = (struct Data* )malloc(sizeof(struct Data));
        if (mpdata[i] == NULL) {
            printf("get pdata false");
        }

        mpdata[i]->size = sizeof(struct Data);
        memcpy(mpdata[i]->name, "Data", sizeof("Data"));

        /* 产生128-65536的随机数 */
        int rand_number_1 = rand() % (65536-128+1) + 128;

        mpdata[i]->data_node = (struct DataNode* )malloc(sizeof(struct DataNode));
        if (mpdata[i]->data_node == NULL) {
            printf("get pdata->data_node false");
			return;
        }
        mpdata[i]->data_node->count = malloc_conut;

        /* pdata->data_node->list */
        mpdata[i]->data_node->list = (char* )malloc(rand_number_1);
        if (mpdata[i]->data_node->list == NULL) {
            printf("get pdata->data_node->list false");
			return;
        }

        /* mpdata[i]->data_node->list = (void* )"this is list,but test memory pool \n"; */
        if (rand_number_1 > strlen(list)) {
            memcpy(mpdata[i]->data_node->list, list, strlen(list));
        }


        /* pdata->data_node->pdata_node */
        mpdata[i]->data_node->pdata_node = (struct eDataNode* )malloc(sizeof(struct eDataNode));
        if (mpdata[i]->data_node->pdata_node == NULL) {
            printf("get mpdata[i]->data_node->pdata_node false");
			return; 
        }


        /* pdata->data_node->pdata_node->e_data */
        mpdata[i]->data_node->pdata_node->e_data = (char* )malloc(rand_number_1);
        if (mpdata[i]->data_node->pdata_node->e_data == NULL) {
            printf("get mpdata[i]->data_node->pdata_node false"); 
			return;
        }

        /* mpdata[i][i]->data_node->pdata_node->e_data = (void* )"end of zhe list,check data is true \n"; */
        if (rand_number_1 > strlen(e_data)) {
            memcpy(mpdata[i]->data_node->pdata_node->e_data, e_data, strlen(e_data)); 
        }

		/* DisplayPool(mpTDataPool); */
		/* printf("testlog1 malloc_conut = %d\n", malloc_conut); */

    }
    GetSysTimeString(timebu_1,sizeof(timebu_1), 8);
    
    for (int j = 0; j < NumberData; j++) {
		free(mpdata[j]->data_node->pdata_node->e_data);
		free(mpdata[j]->data_node->pdata_node);
		free(mpdata[j]->data_node->list);
		free(mpdata[j]->data_node);
		free(mpdata[j]);
    }
    printf("testlog1 mpdata malloc later = %s \n", timebu_1);
    return;
}

int main()
{
    int ret = 0;
    
    MemPoolInit(ret);
	
    //LogInit(0, NULL, NULL);
    printf("开始测试 starting test memory pool\n");

    pthread_t tids[NUM_THREADS];
	struct thread_data td[NUM_THREADS];

    pthread_create(&tids[0], NULL, (void* )testLog, NULL);
    pthread_create(&tids[1], NULL, (void* )testLog1, NULL);
    
	for (int j=0; j<NUM_THREADS; j++) {

		pthread_join(tids[j], NULL);

	}
	
    pthread_exit(NULL);
    
	MemPoolDestory(ret);
    
	return 0;
}


