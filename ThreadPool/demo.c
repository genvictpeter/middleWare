#define THREAD 2
#define QUEUE  10

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>

#include "threadPool.h"

int tasks = 0, done = 0;
pthread_mutex_t lock;

void dummy_task(void *arg) {
    usleep(10000);
    pthread_mutex_lock(&lock);
    printf("done = %d \n", done);
    done++;
    pthread_mutex_unlock(&lock);
}

int main(int argc, char **argv)
{
    ThreadPool *pool;

    pthread_mutex_init(&lock, NULL);

    assert((pool = ThreadPoolCreate(THREAD, QUEUE)) != NULL);
    fprintf(stderr, "Pool started with %d threads and "
            "queue size of %d\n", THREAD, QUEUE);

/*     while(ThreadPoolAppend(pool, &dummy_task, NULL) == 0) {
        pthread_mutex_lock(&lock);
        tasks++;
        pthread_mutex_unlock(&lock);
    } */
    for (int i = 0; i < QUEUE*20; i++) {
        sleep(1);
        if (ThreadPoolAppend(pool, &dummy_task, NULL) != 0) {
            printf("i this is error\n");
            return -1;
        }
    }

/*     for (int j = 0; j < QUEUE; j++) {
        if (ThreadPoolAppend(pool, &dummy_task, NULL) != 0) {
            printf("j this is error\n");
            return -1;
        }
    } */

    fprintf(stderr, "Added %d tasks\n", tasks);

    while((tasks / 2) > done) {
        usleep(10000);
    }
    assert(ThreadPoolDestroy(pool, 2) == 0);
    fprintf(stderr, "Did %d tasks\n", done);

    return 0;
}
