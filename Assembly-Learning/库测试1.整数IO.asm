TITLE Library Test #1: Integer I/O 	(TestLib1.asm)
;Tests the Clrscr, Crlf, DumpMem, ReadInt,
;SetTextColor, WaitMsg, WriteBin, WriteHex,
;and WriteString procedures.

INCLUDE Irvine32.inc

.data
arrayD DWORD 1000h,2000h,3000h
prompt1 BYTE "Enter a 32-bit signed integer: ",0
dwordVal DWORD ?

.code
main PROC
;ʹ使用DumpMem过程显示数组的内容
	mov eax,yellow+(blue*16)
	call SetTextColor
	call Clrscr						;清除屏幕

;设置文本颜色为蓝底黄色
	mov esi,OFFSET arrayD			;起始偏移地址
	mov ecx,LENGTHOF arrayD			;dwordval中元素的数目
	mov ebx,TYPE arrayD				;双字的大小
	call DumpMem					;显示内存内容
	call Crlf						;换行
	
;提示用户输入一个十进制整数
	mov edx,OFFSET prompt1
	call WriteString
	call ReadInt					;输入整数
	mov dwordVal,eax				;保存到一个变量中
	
;以十进制、十六进制和二进制数显示EAX中的整数
	call Crlf						;换行
	call WriteInt					;以有符号十进制数格式显示
	call Crlf						
	call WriteHex					;以十六进制数格式显示
	call Crlf						
	call WriteBin					;以二进制数格式显示
	call Crlf
	call WaitMsg					;"Press any key..."
	
;将控制台窗口设为默认颜色
	mov eax,lightGray+(black*16)
	call SetTextColor
	call Clrscr						;清除屏幕
	exit
main ENDP
END main