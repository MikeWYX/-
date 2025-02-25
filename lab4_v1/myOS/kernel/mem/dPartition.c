#include "../../include/myPrintk.h"


//dPartition 是整个动态分区内存的数据结构
typedef struct dPartition{
	unsigned long size;
	unsigned long firstFreeStart; 
} dPartition;	//共占8个字节

#define dPartition_size ((unsigned long)0x8)

void showdPartition(struct dPartition *dp){
	myPrintk(0x5,"dPartition(start=0x%x, size=0x%x, firstFreeStart=0x%x)\n", dp, dp->size,dp->firstFreeStart);
}

// EMB 是每一个block的数据结构，userdata可以暂时不用管。
typedef struct EMB{
	unsigned long size;
	union {
		unsigned long nextStart;    // if free: pointer to next block
        unsigned long userData;		// if allocated, belongs to user
	};	                           
} EMB;	//共占8个字节

#define EMB_size ((unsigned long)0x8)

void showEMB(struct EMB * emb){
	myPrintk(0x3,"EMB(start=0x%x, size=0x%x, nextStart=0x%x)\n", emb, emb->size, emb->nextStart);
}


unsigned long dPartitionInit(unsigned long start, unsigned long totalSize){
	/*功能：初始化内存。
	1. 在地址start处，首先是要有dPartition结构体表示整个数据结构(也即句柄)。
	2. 然后，一整块的EMB被分配（以后使用内存会逐渐拆分），在内存中紧紧跟在
	dP后面，然后dP的firstFreeStart指向EMB。
	3. 返回start首地址(也即句柄)。
	注意有两个地方的大小问题：
		第一个是由于内存肯定要有一个EMB和一个dPartition，totalSize肯定要比
		这两个加起来大。
		第二个注意EMB的size属性不是totalsize，因为dPartition和EMB自身都需
		要要占空间。
	
	*/
	//内存不足
	if (totalSize < dPartition_size + EMB_size) {
        return 0; 
    }

    dPartition *dp = (dPartition *)start;
    dp->size = totalSize;
    dp->firstFreeStart = start + dPartition_size;

    EMB *first_emb = (EMB *)(dp->firstFreeStart);
    first_emb->size = totalSize - dPartition_size - EMB_size;
    first_emb->nextStart = 0;

    return start;

}


void dPartitionWalkByAddr(unsigned long dp){
	/*功能：本函数遍历输出EMB 方便调试
	1. 先打印dP的信息，可调用上面的showdPartition。
	2. 然后按地址的大小遍历EMB，对于每一个EMB，可以调用上面的showEMB输出其信息

	*/
	dPartition *dp_start = (dPartition *)dp;
    showdPartition(dp_start);

	//遍历emb
    unsigned long emb_addr = dp_start->firstFreeStart;
    while (emb_addr != 0) {
        EMB *emb = (EMB *)emb_addr;
        showEMB(emb);
        emb_addr = emb->nextStart;
    }

}


//=================firstfit, order: address, low-->high=====================
/**
 * return value: addr (without overhead, can directly used by user)
**/

unsigned long dPartitionAllocFirstFit(unsigned long dp, unsigned long size){
	// TODO
	/*功能：分配一个空间
	1. 使用firstfit的算法分配空间，
	2. 成功分配返回首地址，不成功返回0
	3. 从空闲内存块组成的链表中拿出一块供我们来分配空间(如果提供给分配空间的
	内存块空间大于size，我们还将把剩余部分放回链表中)，并维护相应的空闲链表
	以及句柄
	注意的地方：
		1.EMB类型的数据的存在本身就占用了一定的空间。

	*/
	if (size <= 0) // illegal alloc size
		return 0;

	dPartition *dp_start = (dPartition *)dp;

	//8字节对齐
	size = (size + 7) & (~7);

    unsigned long emb_addr = dp_start->firstFreeStart;
    unsigned long pre_addr = 0;
    while (emb_addr) {
        EMB *emb = (EMB *)emb_addr;
        if (emb->size >= size + EMB_size) { //找到足够大的内存
            if (emb->size >= size + 2 * EMB_size) { //内存大小可以被切割
                EMB *new_block = (EMB *)(emb_addr + EMB_size + size);
                new_block->size = emb->size - size - EMB_size;
                new_block->nextStart = emb->nextStart;

                emb->size = size;
                emb->nextStart = emb_addr + EMB_size + size;

                if (pre_addr == 0) {
                    dp_start->firstFreeStart = emb->nextStart;
                } else {
                    EMB *pre = (EMB *)pre_addr;
                    pre->nextStart = emb->nextStart;
                }
            } else { //使用整块内存
                if (pre_addr == 0) {
                    dp_start->firstFreeStart = emb->nextStart;
                } else {
                    EMB *pre = (EMB *)pre_addr;
                    pre->nextStart = emb->nextStart;
                }
            }
            return emb_addr + EMB_size;
        }

        pre_addr = emb_addr;
        emb_addr = emb->nextStart;
    }

    return 0; //内存不足
}


unsigned long dPartitionFreeFirstFit(unsigned long dp, unsigned long start){
	// TODO
	/*功能：释放一个空间
	1. 按照对应的fit的算法释放空间
	2. 注意检查要释放的start~end这个范围是否在dp有效分配范围内
		返回1 没问题
		返回0 error
	3. 需要考虑两个空闲且相邻的内存块的合并
	
	*/
	start -= EMB_size;
	dPartition *dp_start = (dPartition *)dp;
    if (start <= dp || start >= dp + dp_start->size) {
        return 0; //error
    }

    EMB *freeblock = (EMB *)(start);
    unsigned long emb_addr = dp_start->firstFreeStart;
    unsigned long pre_addr = 0;

    while (emb_addr != 0 && emb_addr < start) {
        pre_addr = emb_addr;
        EMB *emb = (EMB *)emb_addr;
        emb_addr = emb->nextStart;
    }

    //是否能与前驱空闲块合并
    if (pre_addr != 0 && pre_addr + ((EMB *)pre_addr)->size + EMB_size == start) {
        EMB *pre = (EMB *)pre_addr;
        pre->size += freeblock->size + EMB_size;
        freeblock = pre;
    } else if (pre_addr == 0) {
        dp_start->firstFreeStart = start;
    } else {
        EMB *pre = (EMB *)pre_addr;
        pre->nextStart = start;
    }

    // 是否能与后继空闲块合并
    if (freeblock->nextStart != 0 && start + freeblock->size == freeblock->nextStart) {
        EMB *next = (EMB *)(freeblock->nextStart);
        freeblock->size += next->size + EMB_size;
        freeblock->nextStart = next->nextStart;
    }

    return 1; //没问题
}


// 进行封装，此处默认firstfit分配算法，当然也可以使用其他fit，不限制。
unsigned long dPartitionAlloc(unsigned long dp, unsigned long size){
	return dPartitionAllocFirstFit(dp,size);
}

unsigned long dPartitionFree(unsigned long	 dp, unsigned long start){
	return dPartitionFreeFirstFit(dp,start);
}
