#include "log.h"
#include <pthread.h>

#define TAG_ID "LOG"
#define NUM_THREADS 5


struct thread_data{
   int  thread_id;
   char* message;
};


void testLog(void* thread)
{
	struct thread_data* my_data = (struct thread_data* ) thread;
	char number[] = "0123456789";
	
	while(1){
		
		LOG(VERBOSE, TAG_ID, READ_WRITE, "%s:%d ", my_data->message, my_data->thread_id);

		LOG(DEBUG, TAG_ID, READ_WRITE, "%s:%d ", my_data->message, my_data->thread_id);

		LOG(INFO, TAG_ID, READ_WRITE, "%s:%d ", my_data->message, my_data->thread_id);

		LOG(WARNING, TAG_ID, READ_WRITE, "%s:%d ", my_data->message, my_data->thread_id);

		LOG(ERROR, TAG_ID, READ_WRITE, "%s:%d ", my_data->message, my_data->thread_id);

		LOG(ASSERT, TAG_ID, READ_WRITE, "%s:%d ", my_data->message, my_data->thread_id);

		LOG_BUF("start test", number, strlen(number), ERROR, "BUF", READ_WRITE); 				/* sizeof(number) */
	}
	return;
}

int main(int argc,char *argv[])
{
	/* 设置log日志等级，详见日志等级枚举 */
	LogInit(VERBOSE, NULL, NULL);

	pthread_t tids[NUM_THREADS];
	struct thread_data td[NUM_THREADS];

	for (int i=0; i<NUM_THREADS; i++) {
		td[i].message = (char* )"This is message";
		td[i].thread_id = i;
		pthread_create(&tids[i], NULL, (void* )testLog, (void* )&td[i]);
	}

	for (int j=0; j<NUM_THREADS; j++) {
		pthread_join(tids[j], NULL);
	}

	pthread_exit(NULL);
	return 0;
}
