#!/usr/bin/env python

import sys, struct
from Crypto.Hash import SHA256

def main():
	if len(sys.argv) != 4:
		print 'Not the right number of argument. You fail. Tried again.'
		sys.exit(1)

	with open(sys.argv[1]) as file1:
		h1 = SHA256.new(file1.read().strip())
	with open(sys.argv[2]) as file2:
		h2 = SHA256.new(file2.read().strip())

	int1 = h1.digest()
	int2 = h2.digest()
	counter = 0
	for i in range(len(int1)):
		byte = ord(int1[i]) ^ ord(int2[i])
		counter += bin(byte).count("1")
	
	with open(sys.argv[3], 'w') as outfile:
		outfile.write("%x" % counter)

if __name__ == "__main__":
	main()