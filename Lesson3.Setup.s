# Lesson3 设置GDT 切换到保护模式

# GDT全局描述符表是系统中唯一存放段寄存器内容（段描述符）的数组，配合
# 程序进行保护模式下的段寻址。它在操作系统的进程切换中具有重要的意义，
# 可理解为所有进程的总目录表，其中存放着每一个任务局部描述符表（LDT）
# 地址和任务状态段（TSS）地址，用于完成进程中各段的寻址、现场保护和现
# 场恢复。
# IDT保存保护模式下所有中断服务程序的入口地址，类似于实模式下的GDT。

# setup.s                       (C) 1991 Linus Torvalds
# setup.s是一个操作系统加载程序，它的主要作用是利用ROM BIOS中断读取
# 机器系统数据，并将这些数据保存到0x90000开始的位置（覆盖掉bootsect
# 程序所在的地方）。
.code16

.text

.equ SETUPSEG, 0x9020           # 本程序所在的段地址
.equ INITSEG, 0x9000            # 原来bootsect所处的段   
.equ SYSSEG, 0x1000             # system在0x10000(64KB)处
.equ LEN, 54

.global _start, begtext, begdata, begbss, endtext, enddata, endbss

.text
    begtext:
.data
    begdata:
.bss
    begbss:
.text

show_text:
# 显示一段文本
    mov $SETUPSEG, %ax
    mov %ax, %endbss
    mov $0x03, %ah
    xor %bh, %bh
    int $0x10                   
    mov $0x000a, %bx
    mov $0x1301, %ax
	mov $LEN, %cx
	mov $msg, %bp
	int $0x10

# 下面调用BIOS中断将硬件的一些状态保存到0x9000:0000开始的内存处
    ljmp $SETUPSEG, $_start

_start:
# int $0x10 service 3 读光标位置
# 功能号：AH = 03
# BH = 页号
# 返回：
# CH = 扫描开始线
# CL = 扫描结束线
# DH = 行号
# DL = 列号
	mov $INITSEG, %ax               # 这在bootsect已完成，Linus又重新设置了一下
	mov %ax, %ds
	mov $0x03, %ah                  # 读取光标位置
	xor %bh, %bh
	int $0x10
	mov %dx, %ds:0

# int $0x15 service 0x88 取扩展内存大小的值（KB）
# 功能号：AH = 88h
# 返回：
# AX = 从0x1000000(1M)处开始的扩展内存大小(KB)
# 若出错则CF置位，AX = 出错码
    mov $0x88, %ah
    int $0x15
    mov %ax, %ds:2

# int $0x10 service 0xF 取显卡显示模式
# 功能号：AH = 0F
# 返回：
# AH = 字符列数
# AL = 显示模式
# BH = 当前显示页
    mov $0x0f, %ah
    int $0x10
    mov $bx, %ds:4
    mov %ax, %ds:6

# int $0x10 service 0x12 检查显示方式(EGA/VGA)并取参数
# 功能号：BL = 10 AL = 0x12
# 返回：
# BH = 显示状态。0x00-彩色模式，I/O端口=0x3dX；0x01-单色模式，I/O端口=0x3bX
# BL = 安装的显示内存。0x00-64k；0x01-128k；0x02-192k；0x03-256k。
# CX = 显示卡特性参数
    mov $0x12, %ah
    mov $0x10, %BL
    int $0x10
    mov %ax, %ds:8
    mov %bx, %ds:10
	mov %cx, %ds:12

# 复制硬盘参数表信息
# 第一个硬盘参数表的首地址在0x41中断向量处，第二个硬盘参数表的首地址在0x46中断向量处
# 紧接着第一个参数表；每个参数表长度为0x10 Byte

# 第一个硬盘参数表
    mov $0x0000, %ax
    mov %ax, %ds
    lds %ds:4*0x41, %si                 # ds:4*0x41赋给ds:si
    mov $INITSEG, %ax
    mov %ax, %es
    mov $0x0080, %di
    mov $0x10, %cx
    rep movsb                           
    # rep movsb将ds:si赋给es:di；若方向标志DF是0则si加1、di加1，否则si减1、di减1；CX减1，再判断CX决定是否继续

# 第二个硬盘参数表
    mov $0x0000, %ax
    mov %ax, %ds
    lds %ds:4*0x46, %si
    mov $INITSEG, %ax
    mov %ax, %es
    mov $0x0090, %di
    mov $0x10, %cx
    rep movsb 

# 检查第二块硬盘是否存在，如果不存在则清空相应的参数表
# int $0x15, service 0x15
# 功能号：AH = 15h 
# 输入：DL = 驱动器号（0是软盘1，盘符为A；1是软盘2；80h是硬盘1,81h是硬盘2；仅支持两块硬盘）
# 返回：
# AH = 类型码 00-无此盘；01-软驱，无change-line支持；02-软驱（或其他可移动设备），
# 有change-line支持；03-硬盘
# 如果成功 CF = 0；如果失败 CF = 1
    mov $0x1500, %ax
    mov $0x81, %dl 
    int $0x13
    jc no_disk1                         # CF = 1 时跳转，即不是硬盘时
    cmp $3, %ah
    je is_disk1                         # 由上行代码，AH是3时跳转，即是硬盘时

no_disk1:                               # 没有第二块硬盘，清除第二个硬盘参数表
    mov $INITSEG, %ax
    mov %ax, %es
    mov $0x0090, %di
    mov $0x10, %cx
    mov $0x00, %ax
    rep stosb                           # 按字节把AL或AX中的数据装入ES:DI指向的存储单元，若方向标志DF=0则DI加1，否则减1。

is_disk1:
# 下面将切换到保护模式，这是非常重要的一步
# 进行切换到保护模式前的准备工作
    cli                                 # 关中断；从此开始不允许中断
    
# 首先我们将system模块从0x1000:0000移到0x0000:0000处
# bootsect引导程序是将system模块读入到0x10000(64KB)开始的位置。由于当时假设system模块
# 最大长度不会超过0x80000(512KB)，即其末端不会超过内存地址0x90000，所以bootsect会把自己
# 移动到0x90000开始的地方，并把setup加载到它后面。下面这段程序的用途是再把整个system模块
# 移动到0x00000位置，即把从0x10000到0x8ffff的内存数据块(512KB)整块地向内存低端移动了0x10000
# (64KB)的位置。
    mov $0x0000, %ax
    cld                                 # 使方向标志DF = 0

do_move:
    mov %ax, %es
    add $0x1000, %ax
    cmp $0x9000, %ax                    # 判断是否把最后一段(从0x8000段开始的64KB)移动完
    jz end_move                         # 如果移动完，跳转
    mov %ax, %ds                        # ds:si 是源地址（初始是0x1000:0x0）
    sub %di, %di
	sub %si, %si
	mov $0x8000, %cx                   	# 移动0x8000字（64KB字节）
	rep movsw
	jmp do_move

# 加载GDT、IDT等

end_move:
    mov $SETUPSEG, %ax
    mov %ax, %ds                        # ds指向本程序（setup）段
    lgdt gdt_48

# 开启A20地址线，使得OS可以访问64KB以上的内存
    inb $0x92, %al 
    orb $0b00000010, %al
    outb %al, $0x92

# 下面设置并进入32位保护模式运行。首先加载机器状态字，也称控制寄存器CRO，其比特位0置1
# 将导致CPU切换到保护模式，并且运行在特权级0中，即当前特权级CPL=0。此时段寄存器仍然指
# 向与实地址模式中相同的线性地址处（在实地址模式下线性地址与物理内存地址相同）。在设置
# 该比特位后，随后一条指令必须是一条段间跳转指令以用于刷新CPU当前指令队列。因为CPU是在
# 执行一条指令之前就已从内存读取该指令并对其进行解码。然后在进入保护模式以后那些属于实
# 模式的预先取得的指令信息就变得不再有效。而一条段间跳转指令就会刷新CPU的当前指令队列，
# 即丢弃这些无效信息。

    mov %cr0, %eax
    bts $0, %eax                        # 打开保护模式比特位(PE)
    mov %eax, %cr0

    .equ sel_cs0, 0x0008
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    ljmp $sel_cs0, $0                   # 跳转至cs段偏移0处

# 我们已将system模块移动至0x00000开始的地方。所以上句中的偏移地址是0。而段值8已经是保护
# 模式下的段选择符了，用于选择描述符表和描述符表项以及所要求的特权级。

# 全局描述符表开始处。描述符表由多个8字节长的描述符项组成。
gdt_48:
    .word 0x800
    .word 512+gdt, 0x9                  # 给予GDT基地址0x90200

# 这里给出了3个描述符项。
gdt:
# 第1项无用，但必须存在。
    .word 0, 0, 0, 0
# 第2项：系统代码段描述符
    .word 0x07FF                        # 0--2047，因此是2048*4096=8Mb
	.word 0x0000                        # 基地址 = 0
	.word 0x9A00                        # 代码段为只读，可执行。
	.word 0x00C0                        # 颗粒度为4096,32位模式。
# 第3项：系统数据段描述符
    .word 0x07FF                        # 0--2047，因此是2048*4096=8Mb
	.word 0x0000                        # 基地址 = 0    
	.word 0x9200                        # 数据段为可读可写。
	.word 0x00C0                        # 颗粒度为4096,32位模式。

msg:
	.byte 13, 10
	.ascii "You've successfully load the floppy data into RAM"
	.byte 13, 10, 13, 10

.text
endtext:
.data
enddata:
.bss
endbss:
