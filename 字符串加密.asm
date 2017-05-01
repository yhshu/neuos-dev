TITLE Encryption Program					Encrypt.asm

INCLUDE Irvine32.inc
KEY = 239									;1~255之间的任意值
BUFMAX = 128								;缓存区的最大值

.data
sPrompt 	BYTE "Enter the plain text: ",0
sEncrypt	BYTE "Cipher text:			",0
sDecrypt	BYTE "Decrypted:			",0
buffer		BYTE 	BUFMAX+1 DUP(0)
bufSize 	DWORD	?

.code
main PROC
	call InputTheString						;输入明文
	call TranslateBuffer					;加密缓冲区
	mov edx,OFFSET sEncrypt					;显示加密的消息
	call DisplayMessage
	call TranslateBuffer					;解密缓冲区
	mov edx,OFFSET sDecrypt					;显示解密消息
	call DisplayMessage
	exit
main ENDP

;-----------------------------------------------------
InputTheString PROC
;
;Prompts user for a plaintext string. Saves the string
;and its length.
;Receives: nothing
;Returns: nothing
;-----------------------------------------------------
	pushad									;压入32位寄存器
	mov edx,OFFSET sPrompt					;显示提示信息
	call WriteString
	mov ecx,BUFMAX							;最多显示数目
	mov edx,OFFSET buffer					;指向缓存区
	call ReadString							;输入字符串
	mov bufSize,eax							;保存其长度
	call Crlf
	popad
	ret
InputTheString ENDP

;-----------------------------------------------------
DisplayMessage PROC
;Displays the encrypted or decrypted message.
;Receives: EDX points to the message
;Returns: nothing
;-----------------------------------------------------
	pushad
	call WriteString
	mov edx,OFFSET buffer					;显示缓存区内容
	call Crlf
	call Crlf
	popad
	ret
DisplayMessage ENDP

;-----------------------------------------------------
TranslateBuffer PROC
;
;Translates the string by exclusive-ORing each
;byte with the encryption key byte.
;Receives: nothing
;Returns: nothing
;-----------------------------------------------------
	pushad
	mov ecx,bufSize							;循环计数器
	mov esi,0								;缓存区的索引0
L1:
	xor buffer[esi],KEY						;转换一个字节
	inc esi									;指向下一个字节
	loop L1
	popad
	ret
TranslateBuffer ENDP
END main