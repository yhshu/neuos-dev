# Lesson3：将操作系统装载到正确的地址，设置中断向量表（GDT）后切换到保护模式。

# bootsect.s              (C) 1991 Linus Torvalds

# bootsect.s装载的内容分为三部分
# 0x9000:0000 bootsect
# 0x9000:0200 setup
# 0x1000:0000 system

.code16
.equ SYSSIZE, 0x3000    
# system size是要加载的系统模块的长度，单位是节，16字节为1节
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
.equ ROOT_DEV, 0x301      # 指定/dev/fda为系统镜像所在的设备
.equ ENDSEG, SYSSEG + SYSSIZE # system末尾扇区

ljmp $BOOTSEG, $_start   # 修改cs寄存器为BOOTSEG, 并跳转到_start处执行代码

# 第一部分：复制bootsect
# # 初始加载到0x07c00是OS与BIOS的约定；移至0x90000是根据OS的需要安排内存
_start:
    mov $BOOTSEG, %ax    # 将启动扇区从0x07c0:0000(31kb)处移动到0x9000:0000(576kb)
    mov %ax, %ds         # 以下是rep mov的用法
    mov $INITSEG, %ax    # 源地址ds:si = 0x07co:0000
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

print_msg:
# 后续装载系统模块需要装载240个扇区，是之前装载扇区数量的60倍，在此处显示一条信息提示用户等待
    mov $0x03, %ah          # 输出信息之前读取光标位置，中断返回使DH为行，DL为列 
    xor %bh, %bh            # bh = 0
    int $0x10

    mov $30, %cx            # 设定输出长度
    mov $0x0007, %bx        # 通常 page 0, attribute 7
    mov $msg1, %bp
    mov $0x1301, %ax        # 输出字符串，移动光标
    int $0x10               # 输出的内容是从ES:BP取得的

# 第三部分：将整个系统镜像装载到0x1000:0000开始的内存中
    mov $SYSSEG, %ax
    mov %ax, %es            # es作为参数
    call read_it            # 加载240个扇区
    call kill_motor         # 关闭软驱电机

    mov %cs:root_dev, %ax   # Root Device根文件系统设备
    cmp $0, %ax             # 判断根设备号是否为0
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

undef_root:              # 若未找到根设备，死循环（死机）
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
    # 判断是否已经读入全部数据。比较当前所读段是否就是系统数据末端所处的段(ENDSEG)
    # 如果不是就跳转到下面ok1_read标号处继续读数据。否则退出子程序返回。
    mov %es, %ax
    cmp $ENDSEG, %ax        # 判断是否已加载所有数据
    jb ok1_read             # $ENDSEG > %ES时跳转
    ret                     # 当进位标志CF = 1时，jb指令跳转
    
ok1_read:
# 计算和验证当前磁道需要读取的扇区数，放在ax中。
# 根据当前磁道还未读取的扇区以及段内数据字节开始偏移位置，计算如果全部读取这些
# 未读扇区，所读总字节数是否会超过64KB段长度的限制。若超过，则根据此次最多能读入
# 的字节数（64KB-段内偏移位置），计算出此次需要读取的扇区数。
    mov %cs:sectors+0, %ax
    sub sread, %ax
    mov %ax, %cx            # 计算尚未读取的扇区
    shl $9, %cx             # shl指令逻辑左移9位，即乘以512
    add %bx, %cx            # cx = cx * 512字节 + 段内当前偏移值（bx）
    jnc ok2_read            # 如果没有超过64K大，跳转到ok2_read
    je ok2_read
    # 若加上此次将读取的磁道上所有未读扇区时超过64KB，则计算此时最多能读入的字节数：
    # （64KB-段内读偏移位置），再转换成需读取的扇区数；其中0减某数就是取该数64KB的补值
    xor %ax, %ax
    sub %bx, %ax
    shr $9, %ax             # shr指令逻辑右移9位，即除以512

ok2_read:
    # 读当前磁道上指定开始扇区cl和需读扇区数al的数据到es:bx开始处，然后统计当前磁道
    # 上已经读取的扇区数并与磁道最大扇区数sector作比较；如果小于sector说明当前磁道
    # 上的还有扇区未读；于是跳转到ok3_read处继续操作。
    call read_track         # 读当前磁道上指定开始扇区和需读扇区数的数据
    mov %ax, %cx            # cx = 该次操作已读取的扇区数
    add sread, %ax          # 加上当前磁道上已经读取的扇区数
    
    cmp %cs:sectors+0, %ax  # 判断当前磁道是否还有扇区未读
    jnc ok3_read            # 还有扇区未读，跳转到ok3_read
    
    # 若该磁道的当前磁头面所有扇区已经读取，则读该磁道的下一磁头面（1号磁头）上的数据
    # 如果已经完成，则读下一磁道。
    mov $1, %ax
    sub head, %ax
    jne ok4_read            # 如果是0磁头，再读1磁头面上的扇区数据
    incw track              # 否则读下一磁道;incw表明是16位

ok4_read:
    mov %ax, head           # 保存当前磁头号
    xor %ax, %ax              # 清零当前磁道已读扇区数

ok3_read:
    # 如果当前磁道上仍有未读扇区，则首先保存当前磁道已读扇区数，
    # 然后调整存放数据处的开始位置。若小于64KB边界值，则跳转到
    # rp_read处，继续读数据。
    mov %ax, sread           # 保存当前磁道已读扇区数
    shl $9, %cx             # 上次已读扇区数 * 512字节
    add %cx, %bx            # 调整当前段内数据开始位置
    jnc rp_read
    # 否则说明已经读取64KB数据，此时调整当前段，为读下一段数据做准备
    mov %es, %ax
    add $0x1000, %ax        # 将段基址调整为指向下一个64KB内存开始处
    mov %ax, %es
    xor %bx, %bx            # 清零段内数据开始偏移值
    jmp rp_read

# read_track子程序：
# 读当前磁道上指定开始扇区和需读扇区的数据到es:bx开始处。
# 中断 0x13 service 2
# AH = 02
# AL = 需要读取的扇区数量	(1-128 dec.)
# CH = 磁道号  (0-1023)
# CL = 扇区号  (1-17)
# DH = 磁头号  (0-15)
# DL = 驱动器号 (0 = 软盘1/盘符A:, 1 = 软盘2, 80h = 硬盘0, 81h = 硬盘1)
# ES:BX = 缓存区位置
read_track:                 # 读取磁盘的核心程序
    push %ax
	push %bx
	push %cx
	push %dx
	mov track, %dx		    # 取当前磁道号
	mov sread, %cx          # 取当前磁道上已读扇区数
	inc %cx                 # cl = 开始读扇区
	mov %dl, %ch            # ch = 当前磁道号
	mov head, %dx           # 取当前磁头号
	mov %dl, %dh            # dh = 磁头号
	mov $0, %dl             # dl = 驱动器号，0即当前A驱动器
	and $0x0100, %dx        # 按位逻辑与操作；磁头号不大于1
	mov $2, %ah             # 中断功能号ah = 2
	int $0x13               
	jc bad_rt               # 如果出错，跳转bad_rt
	pop %dx
	pop %cx
	pop %bx
	pop %ax
	ret

# 读磁盘操作出错，则执行驱动器复位（磁盘中断功能号0），再跳转到read_track重试
bad_rt:
	mov $0, %ax
	mov $0, %dx
	int $0x13
	pop %dx
	pop %cx
	pop %bx
	pop %ax
	jmp read_track

kill_motor:
    # 这个子程序用于关闭软驱的马达，这样进入内核后就能知道它所处的状态
    # 0x3f2是软盘控制器的一个端口，被称为数字输出寄存器（DOR）端口。
	push %dx
	mov $0x3f2, %dx             # 软驱控制卡的数字输出寄存器DOR端口，只写。
	mov $0, %al                 # 盘符0的A驱动器，关闭FDC，禁止DMA和中断请求，关闭马达
	outsb                       # 将al中的内容输出dx指定的端口去
	pop %dx
	ret
    
sectors:
    .word 0                     # 存放当前启动软盘每磁道的扇区数

msg1:
    .byte 13,10
    .ascii "Bootsect is working ..."
    .byte 13,10,13,10

    # 下面一行是对齐语法，.org 508等价于.=0x1fc；在该处补零，直到地址为 510 的第一扇区的最
    # 后两字节然后在此填充好0xaa55魔术值，BIOS会识别硬盘中第一扇区以0xaa55结尾的为
    # 启动扇区，于是BIOS会装载代码并运行
    .=508

root_dev:# 根文件系统设备号，init/main.c中会用
    .word ROOT_DEV

# 下面是启动盘具有有效引导扇区的标志。仅供BIOS中的程序加载引导扇区时识别使用。它必须
# 位于引导扇区的最后两个字节中。
boot_flag:
    .word 0xAA55

.text
    endtext:
.data
    enddata:
.bss
    endbss:
    
