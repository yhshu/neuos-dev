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

    addr = MAP_NR(addr); //计算出需要的页号
    if (mem_map[addr]--)
        return; //如果该页为被占用状态(mem_map[addr]==1)，清零并返回
    mem_map[addr] = 0;
    panic(Trying to free page !); //不应出现这种情况
}

//释放页表，以及相应的物理页释放，并释放该页目录项占用的物理页
int free_page_tables(unsigned long from, unsigned long size)
{
    unsigned long *pg_tbl;
    unsigned long *pg_dir, nr;

    //检查是否为 4MB 内存边界（该函数仅处理连续的4MB内存块）
    if (from & 0x3fffff)
        panic("free_page_tables called with wrong alignment");
    //如果释放低 4MB 内存，也会引发错误，即from == 0
    if (!from)
        panic("try to free up swapper memory space");
    size = (size + 0x3fffff) >> 22; //计算所占用的目录项数
    //计算目录项地址
    pg_dir = (unsigned long *)((from >> 20) & 0xffc);

    //首先释放页表项，然后释放目录项
    //注意释放的是连续的目录/页表项。因而pg_dir自增即可
    for (; size-- > 0; pg_dir++)
    {
        if (!(*pg_dir & 1)) //说明该目录项没有被使用
            continue;
        pg_tbl = (unsigned long *)(*pg_dir & 0xfffff000);
        //开始释放页表项
        for (nr = 0; nr < 1024; nr++)
        {
            if (*pg_tbl & 1) //此页存在
                free_page(0xfffff000 & *pg_tbl);
            *pg_tbl = 0;
            pg_tbl++;
        }
        //释放页目录所占用的页
        free_page(0xfffff000 & *pg_dir);
        *pg_dir = 0;
    }
    invalidate(); //刷新页表缓存
    return 0;
}

//用来将一页内存放入页表和页目录，即把相应的页表、页目录表填充
//如果要put的物理页在内存中仍然是unset(mem_map值为0)，则先获取可用物理页
//此函数假定pg_dir = 0(硬编码)
//成功时返回该页的物理地址，失败/OOM 时返回0
//参数 page 为页面的物理地址，address为线性地址
unsigned long put_page(unsigned long page, unsigned long address)
{
    unsigned long *pg_tbl, tmp;
    if (page < LOW_MEM || page >= HIGH_MEMORY)
        printk("Trying to put page %x at %x\n", page, address);

    if (mem_map[MAP_NR(page)] != 1)
        printk("mem_map disagrees with %x at %x\n", page, address);

    //计算该线性地址对应的页目录地址
    pg_tbl = (unsigned)((address >> 20) & 0xffc);
    //如果页目录存在则直接取出pg_tbl
    printk("Params: pg_tbl = %x, entry = %x\n", pg_tbl, (address >> 12) & 0x3ff);
    if (*pg_tbl & 1)
    {
        printk("Page table now available\n");
        pg_tbl = (unsigned long)(*pg_tbl & 0xfffff000);
    }
    //否则申请物理页放置页目录
    else
    {
        if (!(tmp = get_free_page()))
        {
            printk("NO FREE PAGE!");
            return 0;
        }
        *pg_tbl = tmp | 7;
        printk("Tmp = %x\n", tmp);
        printk("Page Table = %x\n", *pg_tbl);
        pg_tbl = (unsigned long *)tmp;
    }
    printk("Put Page Success\n");
    pg_tbl[(address >> 12) & 0x3ff] = page | 7;
    return page;
}

//为线性地址 address 申请一个空物理页，失败则返回异常
void get_empty_page(unsigned long address)
{
    unsigned long tmp;
    if (!(tmp = get_free_page()) || !put_page(tmp, address))
    {
        free_page(tmp);
        oom();
    }
    return;
}

//验证目前要操作的页面（线性地址）是否合法以及可以被写入
//若页面存在但是不可写入，则会尝试解除页面保护(un_wp_page)
void write_verify(unsigned long address)
{
    unsigned long page;
    //检查页目录项是否存在
    if (!((page = *((unsigned long *)((address >> 20) & 0xffc))) & 1))
        return;
    //取页表首地址
    page = page & 0xfffff000;
    page += ((address >> 10) & 0xffc);
    if (*(unsigned long *)page & 3 == 1)
    { //页表P = 1, R/W = 0
        un_wp_page((unsigned long *)page);
    }
    return;
}

//解除页面的写入保护write protection
//若页面在1MB以上空间且页面存在，则在FLAG上添加W FLAG并刷新TLB，否则申请
//一个新的页面并复制老页面的内容到新页面Copy on Write
void un_wp_page(unsigned long *table_entry)
{
    unsigned long old_page, new_page;
    old_page = *table_entry & 0xfffff000;
    //页面存在且位于1MB以上
    if (old_page >= LOW_MEM && mem_map[MAP_NR(old_page)] == 1)
    {
        *table_entry |= 2;
        invalidate();
        return;
    }
    //尝试分配新页面，若失败oom
    if (!(new_page = get_free_page())
        oom();
    //页面被共享，因为进行写时复制(COW)之后该页面就变为独立页面，引用次数 -1
    if (old_page >= LOW_MEM)
        mem_map[MAP_NR(old_page)]--;
    *table_entry = new_page | 7;
    invalidate();
    return;
}

//缺页异常时调用
void do_wp_page(unsigned long error_code, unsigned long address)
{
    un_wp_page((unsigned long *)(((address >> 10) & 0xffc) +
                                     ((*(unsigned long *)((address >> 20) & 0xffc))) &
                                 0xfffff000));
}

//缺页异常调用此函数
//缺页异常会调用的主要处理函数,(有进程管理相关代码, 暂时略),如果当前进程executable是空
//或者地址已超出进程数据，则尝试取一个空页面并返回,
//如果失败则尝试共享页面，如果仍然失败，则尝试取一个新的页面(不在内存中的)，再失败则报错OOM
//参数 error_code, 错误号; address 线性地址
void do_no_page(unsigned long error_code, unsigned long address)
{
    unsigned long tmp;
    unsigned long page;

    address &= 0xfffff000;
    if (!(page = get_free_page()))
        oom();
    if (put_page(page, address))
        return;
    free_page(page);
    oom();
}
