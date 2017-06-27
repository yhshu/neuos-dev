# Lesson3：将操作系统装载到正确的地址，设置中断向量表（GDT）后切换到保护模式。

# 装载的内容分为三部分
# 0x9000:0000 bootsect
# 0x9000:0200 setup
# 0x1000:0000 system

.code16
.equ SYSSIZE, 0x3000    # system size是要加载的系统模块的长度。
# 0x3000是196kb，对于当前OS版本空间足够。

.global _start
.global begtext, begdata, begbss, endtext, enddata, endbss # 定义6个全局标识符
.text # 文本段
begtext:
.data # 数据段
begdata:
.bss # 未初始化数据段
begbss:
.text # 文本段

# bootsect对内存的规划
.equ SETUPLEN, 0x04      # Setup 程序占用的扇区数
.equ BOOTSEG, 0x07c0     # bootsector原始地址
.equ INITSEG, 0x9000     # bootsector将有的新地址

.equ SETUPSEG, 0x9020    # setup.s的代码会被移动到这里，是BootSector后一个扇区
.equ SYSSEG, 0x1000      # system 程序的装载地址
.equ ROOTDEV, 0x301      # 指定/dev/fda为系统镜像所在的设备
.equ ENDSEG, SYSSEG + SYSSIZE # system末尾扇区

ljmp $BOOTSEG, $_start   # 修改cs寄存器为BOOTSEG, 并跳转到_start处执行代码

# 第一部分：复制bootsect
# # 初始加载到0x07c00是OS与BIOS的约定。移至0x90000是根据OS的需要安排内存
_start:
    mov $BOOTSEG, %ax    # 将启动扇区从0x07c0:0000(31kb)处移动到0x9000:0000(576kb)
    mov %ax, %ds         # 以下是rep mov的用法
    mov %INITSEG, %ax    # 源地址ds:si = 0x07co:0000
    mov %ax, %es         # 目的地址es:di = 0x9000:0000
    mov $256, %cx        # 移动次数 %cx = 256
    xor %si, %si         # movsw每次移动一个word（2byte），共256*2=512byte，即一个启动扇区的大小
    xor %di, %di
    rep movsw

# 扇区复制后跳转到新扇区：
    ljmp $INITSEG, $go   # 长跳转，切换到CS:IP
go: 
# 代码复制完成后，0x07c00和0x90000位置处有两段完全相同的代码。复制代码本身
# 是要靠指令执行的，执行指令的过程就是CS和IP不断变化的过程。执行到ljmp $INITSEG,$go
# 之前，代码的作用就是复制代码自身；执行ljmp $INITSEG,$go之后，程序就转到执行0x90000
# 这边的代码。跳转到新位置后，应当执行下面的代码而不是从本程序开始死循环。
# ljmp $INITSEG,$go和go: mov %cs.%ax巧妙地实现了到新位置后接着原来的执行序继续执行的目的。

# bootsect复制到新地址后，代码整体位置发生变化，对DS，ES，SS寄存器进行初始化
    mov %cs, %ax         # 通过ax将ds、es、ss设置成与cs相同的位置
    mov %ax, %ds
    mov %ax, %es         # 设置好ES寄存器，为后续输出字符串准备
    mov %ax, %ss
    mov $0xFF00, %sp     
    # ss和sp联合使用构成了栈数据在内存中的位置值
    # 为后面程序的栈操作打下了基础

# 第二部分：将软盘中的setup程序加载到内存中，并且跳转到相应地址执行代码
load_setup:
    # 使用BIOS提供的int0x13中断向量所指向的中断服务程序（磁盘服务程序）
    # 以下是调用中断前传参
    mov $0x0000, %dx     # 选择磁盘号0，磁头号0读取
    mov $0x0002, %cx     # 从2号扇区，0轨道开始读取；扇区是从1开始编号的
    mov $INITSEG, %ax    # ES:BX 指向装载目的地址
    mov %ax, %es         # 使得es=INITSEG
    mov $0x0200, %bx     # INITSEG中的地址512
    mov $02, %ah         # int 13, service 2:读取磁盘扇区
    mov $4, %al          # 读取的扇区数
    int $0x13            # 调用BIOS中断
    jnc demo_load_ok     # 没有异常，加载成功
    mov $0x0000, %dx
	mov $0x0000, %ax	 # int 13, service 0:重置磁盘
	int $0x13
	jmp load_setup		 # 一直重试，直到加载成功

demo_load_ok:
    mov $0x00, %dl
    mov $0x0800, %ax
    int $0x13
    mov $0x00, %ch
    mov %cx, %cs:sectors+0
    mov $INITSEG, %ax
    mov %ax, %es

# 第三部分：将整个系统镜像装载到0x1000:0000开始的内存中
    mov $SYSSEG, %ax
    mov %ax, %es        # es作为参数
    call read_it        # 加载240个扇区
    call kill_motor     # 关闭驱动器

    mov %cs:root_dev, %ax   # Root Device根文件系统设备
    cmp $0, &ax             # 判断根设备号是否为0
    jne root_defined        # ZF = 0时跳转，即ax不为0时，即根设备已设置
    # 根设备未设置
    mov %cs:sectors+0, %bx  
    mov $0x0208, %ax
    # cmp指令执行减法运算但不保存结果，只根据结果设置相关的条件标志位（SF、ZF、CF、OF）
    # cmp指令后往往跟着条件转移指令，实现根据比较的结果产生不同的程序分支的功能。
    cmp $15, %bx         # Sector = 15, 1.2MB软驱
    je root_defined      # ZF = 1时跳转
    mov $0x021c, %ax
    cmp $18, %bx         # Sector = 18, 1.44MB软驱
    je root_defined

undef_root:              # 若未找到根设备，一直循环
    jmp undef_root

root_defined:
    mov %ax, %cs:root_dev+0

# 完成全部到内存的装载，跳转到setup.s，现位于0x9020:0000
    ljmp $SETUPSEG, $0

# bootsect主体到此结束；下面是read_it 和 kill_motor 两个子函数，
# 用来快速读取软盘中的内容，以及关闭软驱电机

# 首先定义一些变量，用于读取软盘信息使用
sread: .word 1 + SETUPLEN   # 当前轨道读取的扇区数
head:  .word 0              # 当前读头
track: .word 0              # 当前轨道

read_it: # 读取软驱
    mov %es, %ax
    test $0x0fff, %ax
    # TEST指令按位进行逻辑与运算，与AND指令的区别是两个操作数不会被改变。
die:
    jne die                 # 如果ES不是在64KB(0x1000)边界上，就在此停止
    xor %bx, %bx            # %bx = $0
rp_read:
    mov %es, %ax
    cmp $ENDSEG, %ax
    jb ok1_read             # $ENDSEG > %ES时跳转
                            # 当进位标志CF = 1时，jb指令跳转
    ret
ok1_read:
    mov %cs:sectors+0, %ax
    sub sread, %ax
    mov %ax, %cx            # 计算尚未读取的扇区
    shl

sectors:
    .word 0

msg1:
    .byte 13,10
    .ascii "This program is working."
    .byte 13,10,13,10

    .=0x1fc
    # 对齐语法，等价于.org 508；在该处补零，直到地址为 510 的第一扇区的最后两字节
    # 然后在此填充好0xaa55魔术值，BIOS会识别硬盘中第一扇区以0xaa55结尾的为启动扇区
    # 于是BIOS会装载代码并运行

root_dev:# 根文件系统设备号
    .word ROOT_DEV

boot_flag:
    .word 0xAA55

.text
    endtext:
.data
    enddata:
.bss
    endbss:
