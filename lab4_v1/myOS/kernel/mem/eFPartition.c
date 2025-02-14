#include "../../include/myPrintk.h"


// eFPartition是表示整个内存的数据结构
typedef struct eFPartition{
	unsigned long totalN;
	unsigned long perSize;  // unit: byte	
	unsigned long firstFree;
}eFPartition;	// 占12个字节

#define eFPartition_size 12

void showeFPartition(struct eFPartition *efp){
	myPrintk(0x5,"eFPartition(start=0x%x, totalN=0x%x, perSize=0x%x, firstFree=0x%x)\n", efp, efp->totalN, efp->perSize, efp->firstFree);
}

// 一个EEB表示一个空闲可用的Block
typedef struct EEB {
	unsigned long next_start;
}EEB;	// 占4个字节

#define EEB_size 4

void showEEB(struct EEB *eeb){
	myPrintk(0x7,"EEB(start=0x%x, next=0x%x)\n", eeb, eeb->next_start);
}


void eFPartitionWalkByAddr(unsigned long efpHandler){
	/*功能：本函数是为了方便查看和调试的。
	1. 打印eFPartiiton结构体的信息，可以调用上面的showeFPartition函数。
	2. 遍历每一个EEB，打印出他们的地址以及下一个EEB的地址（可以调用上面的函数showEEB）

	*/
	//打印eFPartiiton结构体的信息
	eFPartition* efp = (eFPartition*)efpHandler;
    showeFPartition(efp);

	//遍历每一个EEB，打印出他们的地址以及下一个EEB的地址
    unsigned long addr = efp->firstFree;
    while (addr) {
        EEB* eeb = (EEB*)addr;
        showEEB(eeb);
        addr = eeb->next_start;
    }
}


unsigned long eFPartitionTotalSize(unsigned long perSize, unsigned long n){
	/*功能：计算占用空间的实际大小，并将这个结果返回
	1. 根据参数persize（每个大小）和n个数计算总大小，注意persize的对齐。
		例如persize是31字节，你想8字节对齐，那么计算大小实际代入的一个块的大小就是32字节。
	2. 同时还需要注意“隔离带”EEB的存在也会占用4字节的空间。
		typedef struct EEB {
			unsigned long next_start;
		}EEB;	
	3. 最后别忘记加上eFPartition这个数据结构的大小，因为它也占一定的空间。

	*/

	 //4字节对齐
    perSize = (perSize + 3) & (~3);
    
    return n * (perSize + EEB_size) + eFPartition_size;

}

unsigned long eFPartitionInit(unsigned long start, unsigned long perSize, unsigned long n){
	/*功能：初始化内存
	1. 需要创建一个eFPartition结构体，需要注意的是结构体的perSize不是直接
	传入的参数perSize，需要对齐。结构体的next_start也需要考虑一下其本身的
	大小。
	2. 就是先把首地址start开始的一部分空间作为存储eFPartition类型的空间
	3. 然后再对除去eFPartition存储空间后的剩余空间开辟若干连续的空闲内存块，
	将他们连起来构成一个链。注意最后一块的EEB的nextstart应该是0
	4. 需要返回一个句柄，也即返回eFPartition *类型的数据
	注意的地方：
		1.EEB类型的数据的存在本身就占用了一定的空间。
	*/
	if (perSize <= 0) // illegal perSize
		return 0;

	//4字节对齐
	perSize = (perSize + 3) & (~3);

	//初始化eFPartition结构体
    eFPartition* efp = (eFPartition*)start;
    efp->totalN = n;
    efp->perSize = perSize;
    efp->firstFree = start + eFPartition_size;
    
    //初始化EEB链表
    unsigned long addr = efp->firstFree;
    for(int i = 0; i < n; i++) {
        EEB* eeb = (EEB*)addr;
        eeb->next_start = addr + perSize + EEB_size;
        addr = eeb->next_start;
    }
    ((EEB*)addr)->next_start = 0; 

    return start;

}


unsigned long eFPartitionAlloc(unsigned long EFPHandler){
	/*功能：分配一个空间
	1. 本函数分配一个空闲块的内存并返回相应的地址，EFPHandler表示整个内存的
	首地址
	2. 事实上EFPHandler就是我们的句柄，EFPHandler作为eFPartition *类型的
	数据，其存放了我们需要的firstFree数据信息
	3. 从空闲内存块组成的链表中拿出一块供我们来分配空间，并维护相应的空闲链
	表以及句柄
	注意的地方：
		1.EEB类型的数据的存在本身就占用了一定的空间。

	*/
	eFPartition* efp = (eFPartition*)EFPHandler;

	//无剩余空闲块
    if (efp->firstFree == 0) return 0;

	//更新firstFree数据信息
    EEB* eeb = (EEB*)(efp->firstFree);
    unsigned long free_block = efp->firstFree;
    efp->firstFree = eeb->next_start;

	//返回空闲块开始的地址
    return free_block + EEB_size;
}


unsigned long eFPartitionFree(unsigned long EFPHandler,unsigned long mbStart){
	/*功能：释放一个空间
	1. mbstart将成为第一个空闲块，EFPHandler的firstFree属性也需要相应大的更新。
	2. 同时我们也需要更新维护空闲内存块组成的链表。
	
	*/
	
    eFPartition *efp = (eFPartition *)EFPHandler;

    //若mbStart非法直接返回
    if (mbStart < EFPHandler + sizeof(eFPartition) || mbStart > EFPHandler + eFPartitionTotalSize(efp->perSize, efp->totalN)) {
        return 0;
    }

    //寻找回收内存前后的最近空闲块
    unsigned long addr = efp->firstFree;
    unsigned long addr_pre = 0;
    unsigned long addr_next = 0;

    while (addr) {
        EEB* eeb = (EEB *)addr;
        if (addr < mbStart)
            addr_pre = addr;
        else if (addr > mbStart) {
            addr_next = addr;
            break;
        }
        addr = eeb->next_start;
    }

    //释放空间
    EEB* eeb = (EEB *)mbStart;

    //连接前驱空闲块和被释放的
    if (addr_pre) {
        EEB *eeb_pre = (EEB *)addr_pre;
        eeb_pre->next_start = mbStart;
    }
    //若没有前驱空闲块则回收内存为firstfree
    else {
        efp->firstFree = mbStart;
    }

	//连接后继空闲块，若没有addr_next保持初始值0，故可直接赋值
    eeb->next_start = addr_next;

    return 1;
}
