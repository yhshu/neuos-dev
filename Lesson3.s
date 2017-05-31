# 将操作系统装载到正确的地址，设置GDT后切换到保护模式。
.code16
.equ SYSSIZE, 0x3000    # system size是要加载的系统模块的长度。
# 0x3000是196kb，对于当前版本空间足够。

.global _start
.global begtext, begdata, begbss, endtext, enddata, endbss # 定义6个全局标识符
.text # 文本段
begtext:
.data # 数据段
begdata:
.bss # 未初始化数据段
begbss:
.text # 文本段

.equ SETUPLEN,0x04      # Setup 程序占用的扇区数
.equ BOOTSEG,0x07c0     # bootsector原始地址
.equ INITSEG,0x9000     # bootsector被移至此地址
.equ SETUPSEG,0x9020    # setup.s的代码会被移动到这里，是BootSector后一个扇区
.equ SYSSEG,0x1000      # system 程序的装载地址
.equ ROOTDEV,0x301      # 指定/dev/fda为系统镜像所在的设备
.equ ENDSEG,SYSSEG+SYSSIZE

ljmp $BOOTSEG, $_start  # 修改cs寄存器为BOOTSEG, 并跳转到_start处执行代码

_start:
    mov $BOOTSEG,%ax    # 将启动扇区从0x07c0:0000处移动到0x9000:0000
    mov %ax,%ds         # 以下是rep mov的用法
    mov %INITSEG,%ax    # 源地址ds:si = 0x07co:0000
    mov %ax,%es         # 目的地址es:di = 0x9000:0000
    mov $256,%cx        # 移动次数 %cx = 256
    xor %si,%si         # movsw每次移动一个word（2byte），共256*2=512byte，即一个启动扇区的大小
    xor %di,%di
    rep movsw

    ljmp $INITSEG,$go   # 长跳转，切换CS:IP
go:
    mov %cs,%ax         # 对DS，ES，SS寄存器进行初始化
    mov %ax,%ds
    mov %ax,%es
    mov %ax,%ss
    mov $0xFF00,%sp     # 设置栈

load_setup:
    # 将软盘内容加载到内存中，并且跳转到相应地址执行代码
    mov $0x0000,%dx     # 选择磁盘号0，磁头号0进行读取
    mov $0x0002,%cx     # 从2号扇区，0轨道开始读取；扇区是从1开始编号的
    mov $INITSEG,%ax    # ES:BX 指向装载目的地址
    mov %ax,%es
    mov $0x0200,%bx
    mov $02,%ah         # int 13,ah = 2，读取磁盘扇区
    mov $4,%al          # 读取的扇区数
    int $0x13           # 调用BIOS中断
    jnc demo_load_ok
    mov $0x0000, %dx
	mov $0x0000, %ax	# 重置磁盘
	int $0x13
	jmp load_setup		# 一直重试，直到加载成功

demo_load_ok:
    mov $0x00,%dl
    mov $0x0800,%ax
    int $0x13
    mov $0x00,%ch
    mov %cx,%cs:sectors+0
    mov $INITSEG,%ax
    mov %ax,%es

print_msg:
# 显示一段信息
    mov $0x03,%ah       # 输出信息之前读取光标位置，中断返回使DH为行，DL为列 
    xor %bh,%bh         # bh = 0
    int $0x10

    mov $20,%cx         # 设定输出长度
    mov $0x0007,%bx     
    mov $msg1,%bp
    mov $0x1301,%ax     # 输出字符串，移动光标
    int $0x10           # 输出的内容是从ES:BP取得的

# 将整个系统镜像装载到0x1000:0000开始的内存中
    mov $SYSSEG,%ax
    mov %ax,%es         # es作为参数
    call read_it
    call kill_motor

    mov %cs:root_dev,%ax
    

sectors:
    .word 0

msg1:
    .byte 13,10
    .ascii "This program is working."
    .byte 13,10,13,10

    .=0x1fc
    # 等价于 .org 508

root_dev:
    .word ROOT_DEV

boot_flag:
    .word 0xAA55

.text
    endtext:
.data
    enddata:
.bss
    endbss: