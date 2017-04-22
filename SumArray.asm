TITLE Summing an Array				(SumArray.asm)

INCLUDE Irvine32.inc

.data
intarray WORD 100h,200h,300h,400h

.code
main PROC
	movedi,OFFSET intarray