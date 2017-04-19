TITLE Data Transfer Examples		(Moves.asm)

INCLUDE Irvine32.inc

.data
val1 DWORD 1000h
val2 DWORD 2000h
arrayB BYTE 10h,20h,30h,40h,50h
arrayW WORD 100h,200h,300h
arrayH DWORD 10000h,20000h

.code
main PROC
;MOVZX零扩展
	mov bx,0A69Bh			
	movzx eax,bx					;EAX = 0000A69Bh
	movzx edx,bl					;EDX = 0000009Bh
	movzx cx,bl 					;CX = 009Bh

;MOVSX符号扩展
	mov bx,0A69Bh
	movsx eax,bx					;EAX = FFFFA69Bh
	movsx edx,bl					;EDX = FFFFFF9Bh
	mov bl,7Bh
	movsx cx,bl						;CX = 007Bh

;内存到内存的交换
	mov ax,val1						;AX = 1000h
	xchg ax,val2					;AX = 2000h, val2 = 1000h
	mov val1,ax						;val1 = 2000h
	
;ֱ直接偏移寻址（字节数组）
	mov al,arrayB					;AL = 10h
	mov al,[arrayB+1]				;AL = 20h
	mov al,[arrayB+2]				;AL = 30h

;ֱ直接偏移寻址（字数组）
	mov ax,arrayW					;AX = 100h
	mov ax,[arrayW+2]				;AX = 200h

;直接偏移寻址（双字数组）
	mov eax,arrayD					;EAX = 10000h
	mov eax,[arrayD+4]				;EAX = 20000h
	mov eax,[arrayD+TYPE arrayD]	;EAX = 20000h
	
	exit
main ENDP
END main