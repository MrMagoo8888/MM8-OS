target remote :1234
set architecture i8086
set disassembly-flavor intel
set pagination off
break *0x7c00
break *0x500
continue
layout asm