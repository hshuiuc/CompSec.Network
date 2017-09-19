#!/usr/bin/env python

import sys

def main():
	if len(sys.argv) != 4:
		print 'Not the right number of argument. You fail. Tried again.'
		sys.exit(1)

	[ciphertext, key, plaintext] = sys.argv[1:4]
	with open(ciphertext) as file:
		encrypted = file.read().strip()
	with open(key) as file:
		key = file.read().strip()

	key_dict = {}
	for i in range(0, 26):
		key_dict[key[i]] = i

	output = ""
	for idx in range(0, len(encrypted)):
		if 'A' <= encrypted[idx] and encrypted[idx] <= 'Z':
			output += chr(ord('A') + key_dict[encrypted[idx]])
		else:
			output += encrypted[idx]

	with open(plaintext, 'w') as file:
		file.write(output)

if __name__ == "__main__":
	main()