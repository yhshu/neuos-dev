TITLE Copying a String					(CopyStr.asm)

INCLUDE Irvine32.inc
.data
source BYTE "This is the source string",0
target BYTE SIZEOF source DUP(0),0

.code
main PROC
	mov esi,0							;变址寄存器
	mov exc,SIZEOF SOURCE				;循环计数器
L1:
	mov al,source[esi]					;从源中取一个字符
	mov target[esi],al					;将该字符存储在目的中
	inc esi								;移到下一个字符
	loop L1								;重复复制整个字符串
	
	exit
main ENDP
END main
