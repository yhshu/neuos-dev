//实现部分内存管理的功能。

#include <linux/kernel.h>

#define invalidate() __asm__ volatile("mov %%eax, %%cr3" ::"a"(0))
//用于使得TLB（页表缓存）失效，刷新缓存。

unsigned long put_page(unsigned long page, unsigned long address);
//释放物理内存地址addr开始的一页面内存。修改页面映射数组mem_map[]中引用次数信息。

void testoom()
{
    for (int i = 0; i < 20 * 1024 * 1024; i += 4096)
        do_no_page(0, i); //处理缺页异常
    return;
}

void test_put_page()
{
    char *b = 0x100000;
    calc_mem(); //显示当前内存用量
    *b = 'k';
    while (1)
        ;
    return;
}

//将线性地址转换为PTE
//若成功返回物理地址，若失败返回NULL
unsigned long *linear_to_pte(unsigned long addr)
{
    //首先获取页目录表，后面会被用于引用pde
    unsigned long *pde = (unsigned long *)((addr >> 20) & 0xffc);
    if (!(*pde & 1) || pde > 0x1000) //页目录表不存在或地址不在页表范围内
    {
        return 0;
    }
    //页表地址 + 页表索引 = PTE
    pde = (unsigned long*)(*pde & 0xfffff000);
    return pde+()
}
