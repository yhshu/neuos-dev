    .code16 #十六位汇编
    .global _start #程序开始
    .text

    .equ BOOTSEG, 0x07c0 #equ定义常量；被BIOS识别为启动扇区，装载到内存0x07co处
    #此时处于实汇编，内存寻址为 (段地址 << 4 +量) 可寻址的线性空间为 20位

    ljmp $BOOTSEG,$_start #修改cs寄存器为BOOTSEG，并跳转到_start处执行代码

_start:
    mov $BOOTSEG,%ax    #ax = BOOTSEG
    mov %ax,%es         #设置ES寄存器，为输出字符串作准备
    mov $0x03,%ah       #int 10，ah = 03H 执行的功能是获取光标位置和形状
                        #在输出信息前读取光标位置储存在DX里，中断返回的DH为行，DL为列
    xor %bh,%bh         #bh为显示页码  
    int $0x10

    mov     $20,%cx     #设定输出长度 cx = 20
    mov     $0x0007,%bx #bx是页码；page 0, attribute 7 (normal) 设置必要的属性
    #lea    msg1,%bp    #lea是取地址指令，bp是指针寄存器
    mov     $msg1,%bp   
    mov     $0x1301,%ax #写字符，移动光标 ax = 0x1301
    int     $0x10       #使用这个中断0x10时，输出所得的，因而要设置好 ES 和 BP

loop_forever:           #一直循环
    jmp loop_forever

sectors:
    .word 0

msg1:
    .byte 13,10         #13是回车，10是换行
    .ascii "Hello world!"
    .byte 13,10,13,10

    .=0x1fe             #对齐语法，等价于.org 510，表示在该处补0，即第一扇区的最后两字节

#在此填充魔术值，BIOS会识别硬盘中第一扇区，以0xaa55结尾的为启动扇区，于是BIOS会装载

boot_flag:
    .word 0xAA55
