#include "../../include/mem.h"

// 这里实现之前内存管理函数的封装，只对用户提供my_malloc/free接口

unsigned long my_malloc(unsigned long size){
    // 调用实现的dPartition或者是ePartition的alloc
    return dPartitionAlloc(pMemHandler,size);

}

unsigned long free(unsigned long start){
    // 调用实现的dPartition或者是ePartition的free
    return dPartitionFree(pMemHandler,start);
}