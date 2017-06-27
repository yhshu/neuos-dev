TITLE Link Library Test #3				TestLib3.asm
;计算执行嵌套循环用去的时间

INCLUDE Irvine32.inc
OUTER_LOOP_COUNT = 3					;该值根据处理器的速度进行调整

.data
startTime DWORD ?
msg1 BYTE "Please Wait...",0dh,0ah,0	;0dh是回车/行结束，0ah是换行
msg2 BYTE "Elapsed milliseconds: ",0	;流逝的毫秒

.code
main PROC
	mov edx,OFFSET msg1					;返回msg1偏移地址
	call WriteString

;保存起始时间
	call GetMseconds					;毫秒信息返回到eax中
	mov startTime,eax
	mov ecx,OUTER_LOOP_COUNT			;设定循环次数
	
;执行循环
L1:
	call innerLoop						;执行函数
	loop L1

;显示用去的时间
	call GetMseconds
	sub eax,startTime
	mov edx,OFFSET msg2
	call WriteString
	call WriteDec						;以十进制显示32位无符号整数
	call Crlf
	exit
main ENDP

innerLoop PROC
	push ecx
	mov ecx,0FFFFFFFFh
L1:
	mov eax,eax
	loop L1
	pop ecx
	ret
innerLoop ENDP
END main
	