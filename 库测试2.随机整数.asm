TITLE Link Library Test #2			(TestLib2.asm)
;测试Irvine32链接库中的随机数生成过程
INCLUDE Irvine32.inc
TAB = 9

.code
main PROC
	call Randomize					;初始化随机数发生器
	call Rand1						;自定义函数Rand1
	call Rand2
	exit
main ENDP

Rand1 PROC
;生成10个伪随机整数
	mov ecx,10						;循环10次
L1:
	call Random32					;生成随机数
	call WriteDec					;以无符号十进制数格式显示
	mov al,TAB						;水平制表符
	call WriteChar					;显示水平制表符
	loop L1
	call Crlf						;换行
	ret
Rand1 ENDP

Rand2 PROC
;生成10个范围在-50~+49之间的伪随机整数
	mov ecx,10						;循环10次
L1:
	mov eax,100						;0~99之间的值
	call RandomRange				;生成随机数
	sub eax,50						;值的范围在-50~49之间
	call WriteInt					;以有符号十进制数格式显示
	mov al,TAB						;水平制表符
	call WriteChar					;输出水平制表符
	loop L1
	
	call Crlf						;换行
	ret
Rand2 ENDP
END main