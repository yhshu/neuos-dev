;BootSector
org 07c00h  ;程序将被加载到07c00h

.data
    BootMessage db "This procedure should be working properly."

.code
    _start:
        mov eax,cs
        mov ds,eax
        mov es,eax

        call DispStr    ;显示信息
        jmp $   ;无限循环

    DispStr:
        mov eax,BootMessage
        mov bp, eax      ;ES:BP显示字符串的地址，AL=显示输出方式  
        mov ecx, 42      ;ECX显示字符串长度  
        mov eax, 01301h      ;10h下中断，ah=13h功能为在Teletype模式下显示字符串  
        mov ebx, 000ah       ;bh=页码,BL=属性（若al=00h或01h）  
      
        mov dh, 05h  
        mov dl, 05h     ;DH, DL=字符输出坐标的行和列  
  
        int 10h         ;选择10号中断  

        ret

times 510 - ($ - $$) db 0 ; 表示填充 510-($-$$) 这么多个字节的0
;$表示当前指令的地址，$$表示程序的起始地址,即最开始的7c00，$-$$等于本条指令之前的所有字节数。
;510-($-$$)的效果是，填充这些0之后，从程序开始到最后一个0，一共是510个字节。再加上最后的dw两个字节,整段程序的大小就是512个字节，恰好占满一个扇区。  
dw 0xaa55;Boot Sector结束标志