TITLE Scanning an Array						ArrayScan.asm

;扫描数组查找第一个非零值
INCLUDE Irvine32.inc

.data
intArray SWORD 0,0,0,0,1,20,35,-12,66,4,0
noneMsg BYTE "A non-zero value was not found", 0

.code
main PROC
	mov ebx,OFFSET intArray					;指向数组
	mov ecx,LENGTHOF intArray				;循环计数器
	
L1:
	cmp WORD PTR [ebx],0					;值和零比较
	jnz found								;不为零则跳转
	add ebx,2								;指向下一个数字
	loop L1									;继续循环
	jmp notFound							;没有找到
	
found:
	movsx eax,WORD PTR[ebx]
	call WriteInt
	jmp quit

notFound:									;显示没有找到的信息
	mov edx,OFFSET noneMsg
	call WriteString
	
quit:
	call Crlf
	exit
main ENDP
END main