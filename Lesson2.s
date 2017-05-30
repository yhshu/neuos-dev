# 加载软盘中的内容到内存，并执行相应代码。
.code16
.equ SYSSIZE, 0x3000    # system size是要加载的系统模块的长度。
# 0x3000是196kb，对于当前版本空间足够。

.global _start
.global begtext, begdata, begbss, endtext, enddata, endbss 
# 定义6个全局标识符

.text # 文本段
begtext:

.data # 数据段
begdata:

.bss # 未初始化数据段
begbss:

.text # 文本段

.equ BOOTSEG,0x07c0     # bootsector原始地址
.equ INITSEG,0x9000     # bootsector被移至此地址
.equ DEMOSEG,0x1000     # 要调用的函数所在地址

.equ ROOTDEV,0x301      # 指定/dev/fda为系统镜像所在的设备

ljmp $BOOTSEG, $_start  # 修改cs寄存器为BOOTSEG, 并跳转到_start处执行代码

_start:
    mov $BOOTSEG,%ax    
    mov %ax,%es         # 设置ES寄存器，为后续输出字符串做准备
    mov $0x03,%ah       # int 10 03ah 中断
  	xor	%bh, %bh        # mov %bh,$0
	int	$0x10

    mov $20,%cx         # 设定输出长度
    mov $0x0007,%bx     # 设定属性
    mov $msg1,%bp
    mov	$0x1301, %ax    # 输出字符串，移动光标
    int $0x10

load_demo:
    # 将软盘的内容加载的内存中，并跳转到相应地址执行代码
    mov $0x0000,%dx     # 选择磁盘号0，磁头号0进行读取
    mov $0x0002,%cx     # 从2号扇区，0轨道开始读取；扇区是从1号开始标号的
    mov $DEMOSEG,%ax    # ES:BX 是缓存地址指针
    mov %ax,%es
    mov $0x0200,%bx
    mov $02,%ah         # int 13 02ah 中断，读取磁盘扇区
    mov $4,%al          # 读取的扇区数
    jnc demo_load_ok    # 无异常，加载成功
    jmp load_demo       # 一直重试，直至加载成功

demo_load_ok:
    mov $DEMOSEG, %ax
    mov %ax,%ds

    ljmp $0x1020,$0     # 跳至demo program所在处

sectors:
    .word 0

msg1:
    .byte 13,10
    .ascii "This program is working."
    .byte 13,10,13,10

    .=0x1fe
    # 等价于 .org 510

boot_flag:
    .word 0xAA55