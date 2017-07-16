//本程序进行内存分页的管理。实现对主内存区内存页面的动态分配和回收操作。

//以下是在head.s里已经规划好的参数，这里以宏的形式再次给出。
// 0x00000000-0x00100000为物理内存最低的1MB空间，是system所在
#define LOW_MEM 0x100000
#define PAGING_MEMORY (15 * 1024 * 1024)      //剩余15MB空闲物理内存用于分页
#define PAGING_PAGES(PAGING_MEMORY >> 12)     //分页后的页数
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
static unsigned char mem_map[PAGING_PAGES] = {0};

//在当前状态没有进程管理，若内存超限执行panic
static inline void oom()
{
    panic("Out of memory!\n");
}



//对于内核代码和数据所占物理内存区域以外的内存（1MB以上内存区域），内核使用了一个字节
//数组mem_map[]来表示物理内存页面的状态。
