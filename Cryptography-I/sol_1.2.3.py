#!/usr/bin/env python
import sys, urllib, urllib2, binascii
from pymd5 import md5, padding

BLK_SIZE = 16

def get_status(u):
    req = urllib2.Request(u)
    try:
        f = urllib2.urlopen(req)
        return f.code
    except urllib2.HTTPError, e:
        return e.code

def main():
    if len(sys.argv) != 3:
        print 'Wrong usage.'
        sys.exit(1)
    
    with open(sys.argv[1]) as file:
        ciphertext = file.read().strip().decode('hex')

    hex_cipher = bytearray(ciphertext)
    totalBytes = len(hex_cipher)
    hex_plain = bytearray(totalBytes-BLK_SIZE)

    # find each 16B msg block, i.e. iv, m[0]...m[totalMBlocks-1]
    for numBlk in range(totalBytes/BLK_SIZE-2, -1, -1):
        pad_buf = bytearray(BLK_SIZE)             # will be [0, ..., 0x10] for rmb, [0,...,0x10,0x0f] for rmb-1,...
        guess_buf = bytearray(BLK_SIZE)
        
        # find every 16B via guessing, starting from rightmost B each block
        for numByte in range(15+numBlk*BLK_SIZE, numBlk*BLK_SIZE-1, -1):
            pad_counter = 0x10
            guessPosFromRight = BLK_SIZE - (numByte % BLK_SIZE)-1     # for 16B block: [15 14 ... 1 0]
            for i in range(15-guessPosFromRight, 16):
                pad_buf[i] = pad_counter
                pad_counter -= 1

            # find message byte by guessing thru all 0-255 possible values
            for guess in range(256):
                guess_buf[15-guessPosFromRight] = guess

                msgBlk = hex_cipher[numBlk*BLK_SIZE : (numBlk+1)*BLK_SIZE]
                for j in range(BLK_SIZE):
                    msgBlk[j] = msgBlk[j] ^ guess_buf[j] ^ pad_buf[j]
                msgBlk = binascii.hexlify(msgBlk)
                testCipher = binascii.hexlify(hex_cipher[0 : numBlk*BLK_SIZE]) + msgBlk + \
                    binascii.hexlify(hex_cipher[(numBlk+1)*BLK_SIZE : (numBlk+2)*BLK_SIZE])

                if get_status('http://192.17.90.133:9999/mp1/cli91/?'+testCipher) != 500:    
                    # then our guess value was correct
                    hex_plain[numByte] = guess                     # guess_buf retains this value at this pos
                    break

    with open(sys.argv[2], 'w') as file:
        file.write(str(hex_plain))
    
if __name__ == "__main__":
    main()
