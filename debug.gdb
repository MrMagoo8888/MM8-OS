target remote :1234
set architecture i8086
break *0x7c00
continue
layout asm
display/x $sp
display/1wx 0x40