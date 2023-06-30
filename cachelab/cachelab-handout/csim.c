#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>

typedef struct
{
    int valid_bits;         // 有效位
    unsigned  tag;          // 标记位
    int stamp;              // 时间戳 用来实行lru算法
}cache_line;

char* filepath = NULL;
int s , E , b , S ;
int hit = 0 , miss = 0 , eviction = 0;
cache_line** cache = NULL;

void init()
{
    cache = (cache_line**)malloc(sizeof(cache_line * ) * S);  //开一个大空间大指针

    for(int i = 0 ; i < S ; i++)
        *(cache+i) = (cache_line*)malloc(sizeof(cache_line) * E);  // 再在大空间里面开一个个小指针小空间

    for(int i = 0 ; i < S ; i++)
    {
        for(int j = 0 ; j < E ; j++)
        {
            cache[i][j].valid_bits = 0;      // set all valid_bits is zero
            cache[i][j].tag = 0xffffffff;           //no address
            cache[i][j].stamp = 0;            //time is 0;
        }
    }
}
void update(unsigned address)
{
    unsigned address_s =(address >> b) & ((0xffffffff) >> (32 - s));         // 设置组索引
    unsigned address_t = address>>(s + b);                                 //     设置标记位

    for(int i = 0 ; i < E ; i++)
    {
        if((*(cache+address_s)+i)->tag == address_t){
            cache[address_s][i].stamp = 0;              //被使用了 所以置零
            hit++;
            return;
        }
    }
    for(int i = 0 ; i < E ; i++)
    {
        if(cache[address_s][i].valid_bits == 0)
        {
            cache[address_s][i].tag = address_t;
            cache[address_s][i].valid_bits = 1 ;
            cache[address_s][i].stamp = 0;              // 载入数据
            miss++;
            return;
        }
    }

    int max_stamp=0;
    int max_i;
    for(int i = 0 ; i < E ; i++)
    {
        if(cache[address_s][i].stamp >max_stamp)
        {
            max_stamp = cache[address_s][i].stamp;
            max_i = i;
        }
    }

    eviction++;
    miss++;
    cache[address_s][max_i].tag = address_t;                 //  62到剩下的为替换过程
    cache[address_s][max_i].stamp = 0;
}
void time()
{
    for(int i = 0 ; i < S ;i++)
    {
        for(int j = 0 ; j < E ; j++)
        {
            if(cache[i][j].valid_bits == 1)
                cache[i][j].stamp++;
        }
    }
}
int main(int argc,char *argv[])
{
    int opt;
    while((opt = getopt(argc,argv,"s:E:b:t:")) !=-1)   //   处理输入参数
    {
        switch(opt)
        {
            case 's':
                s=atoi(optarg);         // 字符串转数字函数
                break;

            case 'E':
                E=atoi(optarg);
                break;

            case 'b':
                b=atoi(optarg);
                break;

            case 't':
                filepath = optarg;
                break;
        }
    }

    S = 1 << s;     // 设置组数
    init();

    FILE* tracefile=fopen(filepath,"r");
    if(tracefile == NULL)                   //读取追踪文件
    {
        printf("Open file wrong");
        exit(-1);
    }

    char operation;
    unsigned address;
    int size;
    while(fscanf(tracefile," %c %x,%d",&operation,&address,&size)>0)
    {
        switch(operation)
        {
            case 'L':
                update(address);
                break;

            case 'M':
                update(address);

            case 'S':
                update(address);
                break;
        }

        time();
    }

    for(int i = 0 ; i < S ;i++)
        free(*(cache+i));
    free(cache);
    fclose(tracefile);	                // 关闭文件

    printSummary(hit,miss,eviction);
    return 0;
}
