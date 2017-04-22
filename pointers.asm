TITLE Pointers						(Pointers.asm)

INCLUDE Irvine32.inc

;创建用户自定义类型
PBYTE TYPEDEF PTR BYTE				;字节指针
PWORD TYPEDEF PTR WORD				;字指针
PDWORD TYPEDEF PTR DWORD			;双字指针

.data
arrayB BYTE 10h,20h,30h
arrayW WORD 1,2,3
arrayD DWORD 4,5,6

;创建一些变量指针
ptr1 PBYTE arrayB
ptr2 PWORD arrayW
ptr3 PDWORD arrayD

.code
main PROC
;使用指针变量访问数据
	mov esi,ptr1					;ESI是扩展源指针寄存器
	mov al,[esi]					;10h
	mov esi,ptr2
	mov ax,[esi]					;1
	mov esi,ptr3
	mov eax,[esi]					;4
	exit
	
main ENDP
END main