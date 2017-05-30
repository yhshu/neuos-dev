# boot.s 是bootsect.s的框架程序
.globl begtext, begdata, begbss, endtext, enddata, endbss # 全局标识符，供ld86链接使用

.text # 正文段
begtext:
.data # 数据段
begdata:
.bss # 未初始化数据段
begbss:

.text # 正文段
BOOTSEG = 0x07c0 # BIOS加载bootsect代码的原始段地址

entry start # 告知链接程序，程序从start标号处开始执行
start:
    jmpi go,BOOTSEG # 段间跳转；INITSEG指出跳转段地址，标号go是偏移地址

go:
    mov ax,cs # 赋给as为了初始化ds和es
    mov ds,ax
    mov es,ax
    mov [msg1+17],ah
    mov cx,#20 # 共显示20个字符，包括回车换行符
    mov dx,#0x1004 # 字符串将显示在屏幕第17行、第5列
    mov bx,#0x000c # 字符显示属性：红色
    mov bp,#msg1   # 指向要显示的字符串，这是中断的调用要求
    mov ax,#0x1301 # 打印字符串并移动光标到字符串结尾处。
    int 0x10       # BIOS中断调用0x10，功能0x13，子功能01

forever_loop:
    jmp forever_loop

msg1:
    .ascii "Loading system..."
    .byte 13,10 # 13是回车，10是换行

.org 510 # 表示以后语句从地址510(0x1FE)开始存放
    .word 0xAA55 # 有效引导扇区标志，供BIOS加载引导扇区使用

.text
endtext:
.data
enddata:
.bss
endbss:
