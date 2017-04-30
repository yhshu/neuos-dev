TITLE Reversing a String			RevStr.asm
INCLUDE Irvine32.inc

.data
aName BYTE "Abraham Lincoln",0
nameSize =  ($ - aName) -1			;$表示当前地址

.code
main PROC

;把aName中的每个字符都压入堆栈
	mov ecx,nameSize
	mov esi,0						;esi是扩展源指针
L1:
	movzx eax,aName[esi]			;取一个字符
	push eax						
	inc esi							;esi向后移动
	loop L1

;从堆栈中按反序弹出字符
;并存储在aName数组中
	mov ecx,nameSize
	mov esi,0						
L2:
	pop eax							;取一个字符
	mov aName[esi],al				;保存在字符串中
	inc esi
	loop L2

;显示aName
	mov edx,OFFSET aName
	call WriteString
	call Crlf
	exit
main ENDP
END main