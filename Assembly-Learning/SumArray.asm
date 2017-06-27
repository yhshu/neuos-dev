TITLE Summing an Array				(SumArray.asm)

INCLUDE Irvine32.inc

.data
intarray WORD 100h,200h,300h,400h	;int array

.code
main PROC
	mov edi,OFFSET intarray			;取intarray的地址
	mov ecx,LENGTHOF intarray		;循环计数器
	mov ax,0						;累加器清零
L1:
	add ax,[edi]					;加上一个整数，方括号表示取edi存储的数
	add edi,TYPE intarray			;变址寄存器指向下一个整数，TYPE按字节计算变量的单个元素大小
	loop L1							;重复循环直到ECX=0为止
	exit
main ENDP
END main