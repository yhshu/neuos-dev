TITLE Addition and Subtract				(AddSub3.asm)

INCLUDE Irvine32.inc
.data
Rval SDWORD ?
Xval SDWORD 26
Yval SDWORD 30
Zval SDWORD 40

.code
main PROC
;INC and DEC
	mov ax,1000h
	inc ax								;1001h
	dec ax								;1000h
	
;表达式：Rval = -Xval + （Yval - Zval）
	mov eax,Xval
	neg eax								;EAX = -26
	mov ebx,Yval						;EBX = 30
	sub ebx,Zval						;EBX = -10
	add eax,ebx							;EAX = -36
	mov Rval,eax						;Rval = -36
	
;零标志的例子
	mov cx,1
	sub cx,1							;ZF = 1
	mov ax,0FFFFh
	inc ax								;ZF = 1

;符号标志的例子
	mov cx,1
	sub cx,1
	mov ax,0FFFFh
	inc ax

;进位标志的例子
	mov al,0FFh
	add al,1
	