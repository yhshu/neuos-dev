//本程序进行内存分页的管理。实现对主内存区内存页面的动态分配和回收操作。

//以下是在head.s里已经规划好的参数，这里以宏的形式再次给出。
// 0x00000000-0x00100000为物理内存最低的1MB空间，是system所在
#define LOW_MEM 0x100000
#define PAGING_MEMORY (15 * 1024 * 1024)      //剩余15MB空闲物理内存用于分页
#define PAGING_PAGES(PAGING_MEMORY >> 12)     //分页后总页数
#define MAP_NR(addr) (((addr)-LOW_MEM) >> 12) //计算当前物理地址的对应页号
#define USED 100                              //页面被占用标志

//复制一页物理内存(4k)从from到to
#define copy_page(from, to)             \
    __asm__ volatile("cld; rep; movsl;" \
                     : "S"(from), "D"(to), "c"(1024))

//用于使TLB失效，刷新页表缓存
#define invalidate\
__asm__ volatile("mov %%eax, %%cr3" ::"a"(0))

static long HIGH_MEMORY = 0;
void un_wp_page(unsigned long *table_entry);

//Linux v0.11使用内存字节位图bitmap来管理物理页的状态，声明时全部填0.
//每一个字节表示这个物理页面被共享次数
static unsigned char mem_map[PAGING_PAGES] = {
    0,
};

//在当前状态没有进程管理，若内存超限执行panic
static inline void oom()
{
    panic("Out of memory!\n");
}

//对物理页映像进行初始化(mem_map)，将内存使用的部分内存映像初始设置为占用。
//将从start_mem -> end_mem 处的内存初始化为没有被使用(0-1MB不在考虑之内)
//16M内存的PC，未设置虚拟盘RAMDISK的情况下，start_mem通常是4MB，end_mem
//通常是16MB。此时主内存区范围是4MB-16MB。
//>>12 = /4KB(一页的大小)
void mem_init(unsigned long start_mem, unsigned long end_mem)
{
    int i;
    HIGH_MEMORY = end_mem; //物理内存最高处位end_mem
    for (i = 0; i < PAGING_PAGES; i++)
        mem_map[i] = USED; //除了start_mem-end_mem的区域物理页都应为被占用状态
    i = MAP_NR(start_mem);
    unsigned long longth = end_mem - start_mem;
    longth >> 12; //计算需要设置为free的页数
    while (longth-- > 0)
    {
        mem_map[i++] = 0;
    }
    return;
}

//用来显示当前memory用量的一个小函数
void calc_mem(void)
{
    int i, j, k, free = 0;
    long *pg_tbl, *pg_dir;

    for (i = 0; i < PAGING_PAGES; i++)
        if (!mem_map[i])
            free++;
    printk("%d pages free (of %d in total)\n", free, PAGING_PAGES);

    //遍历页表项，如果页面有效则计入有效页面数量
    for (i = 2; i < 1024; i++)
    {
        if (pg_dir[i] & 1) //先检查dir是否存在
        {
            pg_tbl = (long *)(0xfffff000 & pg_dir[i]); //计算pg_tbl地址
            for (j = k = 0; j < 1024; j++)
            {
                if (pg_tbl[j] & 1) //检查页表项是否存在
                {
                    k++;
                }
            }
            printk("PageDir[%d] uses %d pages\n", i, k);
        }
    }
    return;
}

//获取第一个空闲的内存物理页，该函数从mem_map末尾开始遍历，直到遇到某个物理页
//映像为没有被占用的状态，这时计算出该页的物理地址
//初始化该页内容为0，并返回页起始地址；若不存在可用页返回0
unsigned long get_free_page(void)
{
    register unsigned long __res asm("ax");
    __asm__ volatile("std; repne; scasb\n\t"
                     "jne 1f\n\t"
                     "movb $1, 1(%%edi)\n\t"
                     "sall $12, %%ecx\n\t"
                     "addl %2, %%ecx\n\t"
                     "movl %%ecx, %%edx\n\t"
                     "movl $1024, %%ecx\n\t"
                     "leal 4092(%%edx), %%edi\n\t"
                     "rep; stosl;\n\t"
                     "movl %%edx, %%eax\n"
                     "1: cld"
                     : "=a"(__res)
                     : "0"(0), "i"(LOW_MEM), "c"(PAGING_PAGES), "D"(mem_map + PAGING_PAGES - 1)); // 从尾端开始检查是否有可用的物理页
    return __res;
}

//释放一页物理页
//将mem_map中相应的byte置0，以及一些必要的错误检查
//包括是否访问了内核内存，或是否超出物理内存(16MB)边界
void free_page(unsigned long addr)
{
    if (addr < LOW_MEM)
        return; //决不允许操作物理内存低端
    if (addr >= HIGH_MEMORY)
        return; //不可超出可用内存高端

    addr =
}
