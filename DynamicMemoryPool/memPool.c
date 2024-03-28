/**
* @file      memPool.c
* @brief     内存池源文件
*
* 函数定义
* @copyright http://www.avic-intl-sz.cn/
* @author    xh
* @version   1.0.1
* @date      2021-4-28
* @par history:
* | version | date | author | description |
* | ------- | ---- | ------ | ----------- |
* | 1.0.1 | 2021-4-28 | xh | create |
*/

#include "memPool.h"

/* 定义全局变量 */
static mempool_alloc* p_mempool_alloc;

/**
* @brief            		设置内存池能容纳的最大值
* @note  							
* @param[in]  allocator  	内存池指针
* @param[in]  in_size   	内存池大小
* @return     无   	
*/
static void AllocatorMaxFreeSet(mempool_alloc* allocator, int in_size)
{
    int  max_free_index;
    int  size = in_size;

    pthread_mutex_lock(&(allocator->m_tLock));

    max_free_index	= ALIGN(size, allocator->m_boundary_size) >> allocator->m_boundary_index;	/* 计算大小，单位：增量大小 */
	allocator->current_free_index += max_free_index;
    allocator->current_free_index -= allocator->max_free_index;
    allocator->max_free_index = max_free_index;
	
    if (allocator->current_free_index > max_free_index) {
     	allocator->current_free_index = max_free_index;	/* 当前可容纳的内存大小不能超过最大内存大小 */
    }
	
    pthread_mutex_unlock(&(allocator->m_tLock));
}

/**
* @brief            		内存池销毁
* @note  							
* @param[in]  pthis  	    内存池指针
* @return     无   	
*/
static void AllocatorDestroy(void* pthis)
{
	mempool_alloc* allocator = (mempool_alloc* )pthis;
	pthread_mutex_lock(&(allocator->m_tLock));

	int i;
	mempool_block* Next = NULL,*Cur = NULL;

	/* 逐个释放每个链表的资源 */
	for (i = 0; i < allocator->max_index; i++) {
		Next = allocator->free[i];

		while ((NULL != Next->next) && (Next != NULL)) {
			/* 释放链表上的内存块 */
			Cur = Next;
			Next = Next->next;
			free(Cur);
		}
		
		if (NULL != Next) {
			free(Next);
		}
		
	}
	
	free(allocator->free);
	
	pthread_mutex_unlock(&(allocator->m_tLock));
	pthread_mutex_destroy(&(allocator->m_tLock));
	
	free(allocator);

	return;

}

/**
* @brief            	    内存池分配
* @note  							
* @param[in]  pthis  	    内存池指针
* @param[in]  size   		内存池大小
* @return     void*         分配的内存地址   	
*/
static void* AllocatorAlloc(void* pthis, int _size)
{
	mempool_alloc* allocator = (mempool_alloc* )pthis;
    mempool_block* node, **ref;
    int max_index;
    int i, index;
    int size;

    size = ALIGN(_size + MEMNODE_T_SIZE, allocator->m_boundary_size);	/* 转换为4k倍数 */
    if (size < allocator->m_min_alloc) {
    	size = allocator->m_min_alloc;	/* 允许分配的最小内存 */
    }

    index = (size >> allocator->m_boundary_index) - 1;	/* 换算内存大小对应的索引值 */

    if (index > allocator->m_uint32_max) {
        return NULL;	/* 超过单次所能分配的内存大小 */
    }

    if (index <= allocator->max_index) {	
        /* 表明当前的内存池内有内存块可分配 */
        pthread_mutex_lock(&(allocator->m_tLock));

        max_index = allocator->max_index;
        ref = &allocator->free[index];	/* 取链表上的对应索引块 */
        i = index;
		
        while (*ref == NULL && i < max_index) {	
            /* 遍历链表直到找到可分配的内存块 */
            ref++;	
            i++;
        }

        if ((node = *ref) != NULL) {
            if ((*ref = node->next) == NULL && i >= max_index) {	
                /* 如果所分配的块为最大可用内存块,且该链表上只有一个内存块 */
                do{
                    ref--;
                    max_index--;
                } while (*ref == NULL && max_index > 0);

                allocator->max_index = max_index;	/* 重新设置最大可用内存索引 */
            }

            allocator->current_free_index += node->index;	/* 更新内存池还能容纳的内存大小 */
            if (allocator->current_free_index > allocator->max_free_index) {
             	allocator->current_free_index = allocator->max_free_index;	/* 当前可容纳的内存大小不能超过最大内存大小 */
            }
            pthread_mutex_unlock(&(allocator->m_tLock));

            node->next 			= NULL;
            node->m_bData 		= (char *)node + MEMNODE_T_SIZE;	/* 用户使用的内存地址 */
			
#ifdef PRINTF
			node->m_free_flg	= 0;
#endif
            return node->m_bData;
        }
        pthread_mutex_unlock(&(allocator->m_tLock));
    } else if (allocator->free[0]) {	
        /* 超过限定的规则内存大小,则在这里寻找合适的内存块 */
        pthread_mutex_lock(&(allocator->m_tLock));
        ref = &allocator->free[0];	/* 取free[0]头块 */
        while ((node = *ref) != NULL && index > node->index) {	
            /* 遍历free[0]链表,寻找合适的内存块 */
        	ref = &node->next;
        }

        if (node) {	
            /* 找到可用的内存块 */
            *ref = node->next;
            allocator->current_free_index += node->index;
            if (allocator->current_free_index > allocator->max_free_index) {
             	allocator->current_free_index = allocator->max_free_index;
            }

            pthread_mutex_unlock(&(allocator->m_tLock));
            node->next = NULL;
            node->m_bData = (char *)node + MEMNODE_T_SIZE;	/* 用户使用的内存地址 */
			
#ifdef PRINTF
			node->m_free_flg = 0;
#endif
            return node->m_bData;
        }

        pthread_mutex_unlock(&(allocator->m_tLock));
    }
    /* 找不到可分配的内存块则重新向系统申请 */
    if ((node = (mempool_block* )malloc(size)) == NULL) {
    	return NULL;
    }

    node->next = NULL;
    node->index = index;
    node->m_bData = (char *)node + MEMNODE_T_SIZE;
    node->m_pool = allocator;
	
#ifdef PRINTF
	node->m_free_flg = 0;
#endif

    pthread_mutex_init(&(node->m_tLock), NULL);	/* 初始化内存块的互斥锁 */
	
    return node->m_bData;
}

/**
* @brief            	    释放内存块
* @note  							
* @param[in]  block  	    释放的内存块
* @return     无            	
*/
static void AllocatorFree(void* block)
{
	/* (- MEMNODE_T_SIZE)是将用户使用的地址转换为内存块起始地址 */
	mempool_block* node = (mempool_block* )((char* )block - MEMNODE_T_SIZE);
	
#ifdef PRINTF
	if (1 == node->m_free_flg)
	{	/* 重复释放报错 */
		/* perror("refree node"); */
		return ;
	}
#endif

	if (NULL == node) {
		/* perror("null node"); */
		return ;
	}
	
    mempool_block* next = NULL, *freelist = NULL;	/* freelist 保存释放给系统的内存块 */
    int index, max_index;
    int max_free_index, current_free_index;
    mempool_alloc* allocator = node->m_pool;

    pthread_mutex_lock(&(allocator->m_tLock));

    max_index = allocator->max_index;
    max_free_index = allocator->max_free_index;
    current_free_index = allocator->current_free_index;

    do {
        next = node->next;
        index = node->index;

        if (max_free_index != ALLOCATOR_MAX_FREE_UNLIMITED && index > current_free_index) {	
            /* 超过内存池所能容纳的数值 */
            node->next = freelist;
            freelist = node;
        } else if (index < allocator->m_max_index) {	
            /* 未超过最大规则内存块大小 */
            if ((node->next = allocator->free[index]) == NULL && index > max_index) {	
                /* 超过当前最大可分配内存块 */
                max_index = index;	
            }
			
#ifdef PRINTF
			node->m_free_flg = 1;
#endif
            allocator->free[index] = node;	/* 放入链表头 */
			
            if (current_free_index >= index) {
            	current_free_index -= index;	/* 更新可容纳的内存大小 */
            } else {
            	current_free_index = 0;
            }
        } else {
        	/* 超过最大规则内存块大小 */
#ifdef PRINTF
			node->m_free_flg = 1;
#endif
            node->next = allocator->free[0];
			
            allocator->free[0] = node;	/* 放入free[0]链表 */
            if (current_free_index >= index) {
             	current_free_index -= index;	/* 更新可容纳的内存大小 */
            } else {
            	current_free_index = 0;
            }

        }
    } while ((node = next) != NULL);

    allocator->max_index = max_index;
    allocator->current_free_index = current_free_index;

    pthread_mutex_unlock(&(allocator->m_tLock));

    while (freelist != NULL) {	
        /* 释放内存 */
        node = freelist;
        freelist = node->next;
		pthread_mutex_destroy(&(node->m_tLock));
        free(node);
		node = NULL;
    }

}

/**
* @brief            	    打印内存块
* @note  							
* @param[in]  m_pool  	    内存池指针
* @return     无            	
*/
static void DisplayPool(const mempool_alloc* m_pool)
{
	static int nCount = 0;
	mempool_block* node;
	
	printf("******************\n");
	printf("======>%d\n", nCount++);	/* 计数 */
	printf( "当前内存池已有最大链表索引：max_index = %d\n", m_pool->max_index);
    if (m_pool->max_free_index != ALLOCATOR_MAX_FREE_UNLIMITED) {
	    printf("内存池最大容量：max_free_index = %d，单位大小:%dk\n", m_pool->max_free_index, m_pool->m_boundary_size / 1024);
	    printf("内存池剩余容量：current_free_index = %d，单位大小:%dk\n", m_pool->current_free_index, m_pool->m_boundary_size / 1024);
    }

	printf("##################\n");
	for (int i = 0; i < m_pool->m_max_index; i++) {
		printf("[%d]:\t", i);
		if (m_pool->free[i] != NULL) {
			node = m_pool->free[i];
            /* 将链表上的结点都打印出来 */
			do {
				printf("->%d", node->index);
				node = node->next;
			} while(node != NULL);
		}
		printf("\n");
	}
	printf("##################\n");
}

/**
* @brief            	    查询内存
* @note  							
* @param[in]  pthis  	    内存分配指针
* @return     无            	
*/
static void AllocatorSelect(void* pthis)
{
    mempool_alloc* allocator = (mempool_alloc* )pthis;
    pthread_mutex_lock(&(allocator->m_tLock));
    DisplayPool(allocator);
    pthread_mutex_unlock(&(allocator->m_tLock));
    return;
}

/**
* @brief            	    内存池块上锁
* @note  							
* @param[in]  block  	    指向内存块
* @return     无            	
*/
static void AllocatorNodeLock(void* block)
{
	mempool_block* node = (mempool_block* )((char* )block - MEMNODE_T_SIZE);
    pthread_mutex_lock(&(node->m_tLock));
}

/**
* @brief            	    内存池块解锁
* @note  							
* @param[in]  block  	    指向内存块
* @return     无            	
*/
static void AllocatorNodeUnlock(void* block)
{
	mempool_block* node = (mempool_block* )((char* )block - MEMNODE_T_SIZE);
    pthread_mutex_unlock(&(node->m_tLock));
}

/**
* @brief            				创建内存池
* @note  							单位字节
* @param[in]  Size  				内存池最大容量
* @param[in]  MaxIndex   			内存块链表的最大索引
* @param[in]  MaxAlloc	   			允许分配的最小内存块
* @param[in]  ArpUint32Max			允许分配的最大内存块，单位增量大小
* @param[in]  BoundaryIndex			增量指数
* @return     mempool_alloc   	    内存池指针
*/
mempool_alloc* MemPoolCreate(int Size,int MaxIndex,int MinAlloc,int ArpUint32Max,int BoundaryIndex)
{
    mempool_alloc* new_allocator;

    if ((new_allocator = (mempool_alloc* )malloc(SIZEOF_ALLOCATOR_T)) == NULL) {
     	return NULL;
    }

    memset(new_allocator, 0, SIZEOF_ALLOCATOR_T);
    new_allocator->max_free_index = ALLOCATOR_MAX_FREE_UNLIMITED;	/* 内存空间大小 */

	if ((new_allocator->free = (mempool_block** )malloc(MEMNODE_T_SIZE * MaxIndex)) == NULL) {
    	free(new_allocator);
     	return NULL;
    }
	
    memset(new_allocator->free, 0, MEMNODE_T_SIZE * MaxIndex);
	
	new_allocator->m_max_index 		= MaxIndex;
	new_allocator->m_min_alloc 		= MinAlloc;
	new_allocator->m_uint32_max 	= ArpUint32Max;
	new_allocator->m_boundary_index = BoundaryIndex;
	new_allocator->m_boundary_size 	= (1 << BoundaryIndex);
	new_allocator->destory_mempool 	= AllocatorDestroy;
	new_allocator->mempool_alloc 	= AllocatorAlloc;
	new_allocator->mempool_free 	= AllocatorFree;
    new_allocator->mempool_select   = AllocatorSelect;
	new_allocator->block_lock 		= AllocatorNodeLock;
	new_allocator->block_unlock 	= AllocatorNodeUnlock;
    pthread_mutex_init(&(new_allocator->m_tLock), NULL);

	if(ALLOCATOR_MAX_FREE_UNLIMITED != Size) {
		AllocatorMaxFreeSet(new_allocator, Size);
	}

    return new_allocator;
}

/**
* @brief            				按默认创建内存池
* @note  							
* @return     mempool_alloc   	    内存池指针
*/
mempool_alloc* MemPoolCreateDefault(void)
{
    mempool_alloc* new_allocator;

    if ((new_allocator = (mempool_alloc* )malloc(SIZEOF_ALLOCATOR_T)) == NULL) {
     	return NULL;
    }

    memset(new_allocator, 0, SIZEOF_ALLOCATOR_T);
    new_allocator->max_free_index = ALLOCATOR_MAX_FREE_UNLIMITED;	/* 内存空间不作限制 */

	if ((new_allocator->free = (mempool_block** )malloc(MEMNODE_T_SIZE * DEFAULT_MAX_INDEX)) == NULL) {
    	free(new_allocator);
     	return NULL;
    }
	
    memset(new_allocator->free, 0, MEMNODE_T_SIZE * DEFAULT_MAX_INDEX);
	
	new_allocator->m_max_index 		= DEFAULT_MAX_INDEX;
	new_allocator->m_min_alloc 		= DEFAULT_MIN_ALLOC;
	new_allocator->m_uint32_max 	= DEFAULT_UINT32_MAX;
	new_allocator->m_boundary_index = DEFAULT_BOUNDARY_INDEX;
	new_allocator->m_boundary_size 	= (1 << DEFAULT_BOUNDARY_INDEX);
	new_allocator->destory_mempool 	= AllocatorDestroy;
	new_allocator->mempool_alloc 	= AllocatorAlloc;
	new_allocator->mempool_free 	= AllocatorFree;
    new_allocator->mempool_select   = AllocatorSelect;
	new_allocator->block_lock 		= AllocatorNodeLock;
	new_allocator->block_unlock 	= AllocatorNodeUnlock;
	
    pthread_mutex_init(&(new_allocator->m_tLock), NULL);

    return new_allocator;
}

/**
* @brief            				创建内存池
* @note  							
* @param[in]  size  				内存池最大容量
* @param[in]  MaxIndex   			内存块链表的最大索引
* @param[in]  MaxAlloc	   			允许分配的最小内存块
* @param[in]  ArpUint32Max			允许分配的最大内存块大小，单位增量大小
* @param[in]  BoundaryIndex			增量指数
* @return     0                     成功
* @return     其他                  失败	
*/
int MemPoolInitDynamic(int Size, int MaxIndex, int MinAlloc, int ArpUint32Max, int BoundaryIndex)
{
    p_mempool_alloc = MemPoolCreate(Size, MaxIndex, MinAlloc, ArpUint32Max, BoundaryIndex);
    if (p_mempool_alloc == NULL) {
        return -1;
    }
    return 0;
}

/**
* @brief            按默认方式创建内存池
* @note  			
* @return     0     成功
* @return     其他  失败
*/
int MemPoolDefaultInitDynamic(void)
{
    p_mempool_alloc = MemPoolCreateDefault();
    if (p_mempool_alloc == NULL) {
        return -1;
    }
    return 0;
}

/**
* @brief            申请内存
* @param[in]  size  长度
* @return           内存指针
*/
void* MemPoolAllocDynamic(int Size)
{
    void* new_alloc = p_mempool_alloc->mempool_alloc(p_mempool_alloc, Size);
    return new_alloc;
}

/**
* @brief            释放内存
* @param[in]  p  	内存指针
* @return   0		成功
* @return   其他	失败
*/
int MemPoolFreeDynamic(void* p)
{
    p_mempool_alloc->mempool_free(p);
    return 0;
}

/**
* @brief            销毁内存池
* @return   0		成功
* @return   其他	失败
*/
int MemPoolDestoryDynamic(void)
{
    p_mempool_alloc->destory_mempool(p_mempool_alloc);
    return 0;
}

/**
* @brief            查询内存池
* @return   0		成功
* @return   其他	失败
*/
int MemPoolSelectDynamic(void)
{
    p_mempool_alloc->mempool_select(p_mempool_alloc);
    return 0;
}