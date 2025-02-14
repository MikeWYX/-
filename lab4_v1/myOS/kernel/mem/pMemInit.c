#include "../../include/myPrintk.h"
#include "../../include/mem.h"
unsigned long pMemStart;  // 可用的内存的起始地址
unsigned long pMemSize;  // 可用的大小
unsigned long pMemHandler;

void memTest(unsigned long start, unsigned long grainSize) {
    unsigned short *ptr_head, *ptr_tail;
    unsigned short backup_head, backup_tail;
    unsigned short testVal1 = 0xAA55;
    unsigned short testVal2 = 0x55AA;

    if (start < 0x100000){
		start = 0x100000;
	}

    for (ptr_head = (unsigned short *)start; ; ptr_head += grainSize/sizeof(unsigned short)) {
        ptr_tail = ptr_head + (grainSize/sizeof(unsigned short) - 1);
        
        //存入原值
        backup_head = *ptr_head;
        backup_tail = *ptr_tail;

        //覆盖写入0xAA55，再读出并检查是否是0xAA55，若不是则检测结束；
        *ptr_head = *ptr_tail = testVal1;
        if (*ptr_head != testVal1 || *ptr_tail != testVal1) {
            //恢复原值
            *ptr_head = backup_head;
            *ptr_tail = backup_tail;
            break;
        }

        //覆盖写入0x55AA，再读出并检查是否是0x55AA，若不是则检测结束；
        *ptr_head = *ptr_tail = testVal2;
        if (*ptr_head != testVal2 || *ptr_tail != testVal2) {
            //恢复原值
            *ptr_head = backup_head;
            *ptr_tail = backup_tail;
            break;
        }

        //恢复原值
        *ptr_head = backup_head;
        *ptr_tail = backup_tail;
    }

    pMemStart = start;
    pMemSize = (unsigned long)ptr_head - start - grainSize;

    myPrintk(0x7,"MemStart: %x  \n", pMemStart);
    myPrintk(0x7,"MemSize:  %x  \n", pMemSize);
}


extern unsigned long _end;
void pMemInit(void){
	unsigned long _end_addr = (unsigned long) &_end;
	memTest(0x100000,0x1000);
	myPrintk(0x7,"_end:  %x  \n", _end_addr);
	if (pMemStart <= _end_addr) {
		pMemSize -= _end_addr - pMemStart;
		pMemStart = _end_addr;
	}
	
	// 此处选择不同的内存管理算法
	pMemHandler = dPartitionInit(pMemStart,pMemSize);	
}
