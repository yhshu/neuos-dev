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
# AH = 03
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
# AH = 88h
# 返回：
# AX = 从0x1000000(1M)处开始的扩展内存大小(KB)
# 若出错则CF置位，AX = 出错码
    mov $0x88, %ah
    int $0x15
    mov %ax, %ds:2

# int $0x10 service 0xF 取显卡显示模式
# AH = 0F
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
    
# 第二个硬盘参数表