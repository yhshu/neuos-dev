# 加载软盘中的内容到内存，并执行相应代码。
.code16
.equ SYSSIZE, 0x3000

.global _start
.global begtext, begdata, begbss, endtext, enddata, endbss

.text
begtext:

.data
begdata:

.bss
begbss:

.text

.equ BOOTSEG,0x07c0
.equ INITSEG,0x9000
.equ DEMOSEG,0x1000