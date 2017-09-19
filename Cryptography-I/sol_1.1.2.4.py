#!/usr/bin/env python

import sys

def main():
	if len(sys.argv) != 5:
		print 'Not the right number of argument. You fail. Tried again.'
		sys.exit(1)

	[ciphertext, keytext, modulotext, plaintext] = sys.argv[1:5]
	with open(ciphertext) as file:
		encrypted = int(file.read().strip(), 16)
	with open(keytext) as file:
		key = int(file.read().strip(), 16)
	with open(modulotext) as file:
		modulo = int(file.read().strip(), 16)

	output = pow(encrypted, key, modulo)

	with open(plaintext, 'w') as file:
		file.write("%x" % output)

if __name__ == "__main__":
	main()