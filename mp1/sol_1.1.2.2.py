#!/usr/bin/env python

import sys
from Crypto.Cipher import AES

def main():
	if len(sys.argv) != 5:
		print 'Not the right number of argument. You fail. Tried again.'
		sys.exit(1)

	[ciphertext, keytext, ivtext, plaintext] = sys.argv[1:5]
	with open(ciphertext) as file:
		encrypted = file.read().strip().decode('hex')
	with open(keytext) as file:
		key = file.read().strip().decode('hex')
	with open(ivtext) as file:
		iv = file.read().strip().decode('hex')

	decryptor = AES.new(key, AES.MODE_CBC, iv)
	output = decryptor.decrypt(encrypted)

	with open(plaintext, 'w') as file:
		file.write(output)

if __name__ == "__main__":
	main()