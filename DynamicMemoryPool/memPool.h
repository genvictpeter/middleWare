/**
* @file      memPool.h
* @brief     内存池头文件
*
* 包含通用的宏、枚举和结构体等类型定义
* @copyright http://www.avic-intl-sz.cn/
* @author    xh
* @version   1.0.1
* @date      2021-4-28
* @par history:
* | version | date | author | description |
* | ------- | ---- | ------ | ----------- |
* | 1.0.1 | 2021-4-28 | xh | create |
*/

#ifndef __MEM_POOL_H__
#define __MEM_POOL_H__

#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define RC_OK					(0)			/* 成功 */
/* #define ID 						"MemPool"	 日志标签 */ 
#define DEFAULT_MAX_INDEX		(256)		/* 最大链表索引 */
#define DEFAULT_MIN_ALLOC		(8192)		/* 允许分配的最小内存块8K */
#define DEFAULT_UINT32_MAX	    (2048)		/* 单次允许最大的内存块 */
#define DEFAULT_BOUNDARY_INDEX	(12)		/* 2的12次幂为4k的数值转换 */
#define ALLOCATOR_MAX_FREE_UNLIMITED  (0)	/* 表示对内存池大小不作限制 */

#define ALIGN(size, boundary)	(((size) + ((boundary) - 1)) & ~((boundary) - 1))		/* 把size调整为boundary的整数倍 */
#define ALIGN_DEFAULT(size)		ALIGN(size, 8)											/* 按8字节的最小倍数 */
#define SIZEOF_ALLOCATOR_T		ALIGN_DEFAULT(sizeof(mempool_alloc))					/* 内存分配结构大小 */
#define MEMNODE_T_SIZE			ALIGN_DEFAULT(sizeof(mempool_block))					/* 内存块结构大小 */

/* 声明全局变量 */
typedef struct mempool_alloc mempool_alloc;
typedef struct mempool_block mempool_block;

/**
* @brief 内存池块模块
*/
struct mempool_block 
{
	pthread_mutex_t		m_tLock;		/* 线程互斥锁 */
	mempool_block		*next;			/* 下一个块模块 */
	mempool_block		**ref;			/* 引用自己（预留） */
	int					index;			/* 索引和内存块大小,单位为增量大小 */
	mempool_alloc		*m_pool;		/* 指向内存池分配模块 */
	char				*m_bData;		/* 指向free的地址 */
#ifdef PRINTF
	int					m_free_flg;		/* 重复释放标识 */
#endif
};

/**
* @brief 内存池分配模块
*/
struct mempool_alloc 
{
	int 				max_index;				/* 当前内存池中已有的最大内存块链表索引 */
	int 				max_free_index; 		/* 内存池的最大的容量 */
	int 				current_free_index; 	/* 当前内存池的最大容量 */
	pthread_mutex_t 	m_tLock;				/* 线程互斥锁 */
	int 				*owner;					/* 标记属于哪个内存池 */
	mempool_block		**free;					/* 指向一组链表头块，该链表中每个块指向内存块组成的链表 */

	int 				m_min_alloc; 			/* 限定分配的最小规则内存 */
	int 				m_max_index; 			/* 限定能够分配的最大规则内存链表索引 */
	int 				m_uint32_max; 			/* 限定单次所能够分配的最大内存块,单位：增量大小 */
	int 				m_boundary_index;		/* 限定内存块大小递增指数，已2为底的指数 */
	int 				m_boundary_size; 		/* 限定内存块大小的递增值 */

	/**
	* @brief            销毁内存池
	* @param[in]  pthis 内存池指针
	* @return     无
	*/
	void (*destory_mempool)(void* pthis);

	/**
	* @brief            申请内存
	* @param[in]  pthis 内存池指针
	* @param[in]  size  长度
	* @return           内存指针
	*/
	void *(*mempool_alloc)(void* pthis, int size);

	/**
	* @brief            释放内存
	* @param[in]  block 从内存池申请的内存指针
	* @return     无
	*/
	void (*mempool_free)(void* block);

	/**
	* @brief            查询内存
	* @param[in]  pthis 从内存池申请的块
	* @return     无
	*/
	void (*mempool_select)(void* pthis);

	/**
	* @brief            lock内存块
	* @param[in]  block 从内存池申请的块
	* @param[in]  p   	数据指针
	* @return     无
	*/
	void (*block_lock)(void* block);

	/**
	* @brief            unlock内存块
	* @param[in]  block 从内存池申请的块
	* @param[in]  p   	数据指针
	* @return     无
	*/
	void (*block_unlock)(void* block);		
};

		/* 使用函数传参封装的API */

/**
* @brief            				创建内存池
* @note  							
* @param[in]  size  				内存池最大容量
* @param[in]  MaxIndex   			内存块链表的最大索引
* @param[in]  MaxAlloc	   			允许分配的最小内存块
* @param[in]  Uint32Max				允许分配的最大内存块大小，单位增量大小
* @param[in]  BoundaryIndex			增量指数
* @return     mempool_alloc   		内存池指针
*/
mempool_alloc* 
MemPoolCreate(int Size,int MaxIndex,int MinAlloc,int Uint32Max,int BoundaryIndex);

/**
* @brief            				按默认创建内存池
* @note  							单位字节
* @return     mempool_alloc   		内存池指针
*/
mempool_alloc* 
MemPoolCreateDefault(void);


		/* 使用全局变量封装的API */
		
/**
* @brief            				创建内存池
* @note  							
* @param[in]  size  				内存池最大容量
* @param[in]  MaxIndex   			内存块链表的最大索引
* @param[in]  MaxAlloc	   			允许分配的最小内存块
* @param[in]  Uint32Max				允许分配的最大内存块大小，单位增量大小
* @param[in]  BoundaryIndex			增量指数
* @return     0						成功
* @return     其他					失败   		
*/
int 
MemPoolInitDynamic(int Size,int MaxIndex,int MinAlloc,int Uint32Max,int BoundaryIndex);

/**
* @brief            按默认方式创建内存池
* @note  			
* @return   0		成功
* @return   其他	失败   		
*/
int 
MemPoolDefaultInitDynamic(void);

/**
* @brief            申请内存
* @param[in]  size  长度
* @return           内存指针
*/
void* 
MemPoolAllocDynamic(int Size);

/**
* @brief            释放内存
* @param[in]  p  	内存指针
* @return   0		成功
* @return   其他	失败
*/
int 
MemPoolFreeDynamic(void* p);

/**
* @brief            销毁内存池
* @return   0		成功
* @return   其他	失败
*/
int 
MemPoolDestoryDynamic(void);

/**
* @brief            查询内存池
* @return   0		成功
* @return   其他	失败
*/
int 
MemPoolSelectDynamic(void);


		/* 与静态内存池统一接口 */
/**
* @brief            创建内存池
* @note  			  		
* @param[in]  RC  	int返回值0代表成功
*/
#define MemPoolInit(RC) if (RC_OK != (typeof(RC))MemPoolDefaultInitDynamic()) {printf("MemPool Init Failed\n");}

/**
* @brief            销毁内存池
* @note  			  		
* @param[in]  RC  	int返回值0代表成功
*/
#define MemPoolDestory(RC) if (RC_OK != (typeof(RC))MemPoolDestoryDynamic()) {printf("MemPool Destory Failed\n");}

/**
* @brief            申请内存
* @note  			  		
* @param[in]  PTHIS 返回内存地址
* @param[in]  SIZE  申请内存大小
*/
#define MemPoolAlloc(PTHIS, SIZE) {PTHIS = (typeof(PTHIS))MemPoolAllocDynamic(SIZE); if (NULL == PTHIS) {printf("MemPool Alloc Failed\n");}}

/**
* @brief            释放内存
* @note  			  		
* @param[in]  PTHIS 释放的内存地址
* @param[in]  RC    int返回值0代表成功
*/
#define MemPoolFree(RC, PTHIS) if (RC_OK != (typeof(RC))MemPoolFreeDynamic(PTHIS)) {printf("MemPool Free Failed\n");}

/**
* @brief            查询内存池
* @note  			  		
* @param[in]  RC  	int返回值0代表成功
*/
#define MemPoolSelect(RC) if (RC_OK != (typeof(RC))MemPoolSelectDynamic()) {printf("MemPool Select Failed\n");}

#endif

