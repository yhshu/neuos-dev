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
	