#!/usr/bin/env python
import sys, subprocess, binascii
certbldr = __import__('mp1-certbuilder')
from Crypto.Util import number
from fractions import gcd
from cryptography.hazmat.primitives.serialization import Encoding

def getCoprimes(e=65537):
    bitSize = 500
    p1, p2 = -1, -1
    while p1 == p2:
        p1 = number.getPrime(bitSize)
        p2 = number.getPrime(bitSize)
        if(gcd(e, p1-1) != 1 or gcd(e, p2-1) != 1):
            continue
    return p1, p2

def getCRT(b1, b2, p1, p2):
    N = p1 * p2
    invOne = number.inverse(p2, p1)
    invTwo = number.inverse(p1, p2)
    return -(b1 * invOne * p2 + b2 * invTwo * p1) % N

def main():
    # if len(sys.argv) != 2:
    #     print 'Wrong usage.'
    #     sys.exit(1)
    e = 65537
    p = number.getPrime(1024)
    assert(p.bit_length() == 1024)
    q = number.getPrime(1024)
    assert(q.bit_length() == 1024)
    assert( (p*q).bit_length() == 2047 )

    privkey, pubkey = certbldr.make_privkey(p, q)
    certificate = certbldr.make_cert('cli91', pubkey)
    serial_num = certificate.serial_number
    prefix_string = certificate.tbs_certificate_bytes[:256]
    test = open("test.cer", "w")
    test.write(certificate.public_bytes(Encoding.DER))
    test.close()
    assert(len(prefix_string) % 64 == 0)
    prefix_file = open("prefix", "w")
    prefix_file.write(prefix_string)
    prefix_file.close()
    subprocess.call(["./fastcoll", "-p", "prefix", "-o", "certA", "certB"])
    certA = open("certA", "r")
    certB = open("certB", "r")
    certA_text = certA.read()
    certB_text = certB.read()
    certA.close()
    certB.close()
    certA_text = certA_text[256:]
    certB_text = certB_text[256:]
    assert(len(certA_text) == 128)
    assert(len(certB_text) == 128)
    b1 = binascii.hexlify(bytearray(certA_text))
    b2 = binascii.hexlify(bytearray(certB_text))
    b1 = int(b1, 16)
    b2 = int(b2, 16)
    b1 = b1 << 1024
    b2 = b2 << 1024
    assert(b1 % (2**1024) == 0)
    assert(b2 % (2**1024) == 0)
    p1, p2 = getCoprimes(e=65537)
    b0 = getCRT(b1, b2, p1, p2)
    k = 0
    b = b0 + (k*p1*p2)
    q1 = 0
    q2 = 0
    n1 = 0
    n2 = 0
    less_than = 0
    while(1):
    if(b >= 2**1024 or less_than):
        p1, p2 = getCoprimes(e=65537)
        b0 = getCRT(b1, b2, p1, p2)
        k = 0
        less_than = 0
        b = b0 + (k*p1*p2)
    q1 = (b1 + b)/p1
    q2 = (b2 + b)/p2
    if(number.isPrime(q1) and number.isPrime(q2)):
        if(gcd(q1-1, e) == 1 and gcd(q2-1, e) == 1):
            if(q1.bit_length() < 256 or q2.bit_length() < 256):
                less_than = 1
            elif(p1.bit_length() < 256 or p2.bit_length() < 256):
                less_then = 1
            else:
                n1 = b1 + b
                n2 = b2 + b
                if(n1.bit_length() >= 2048 or n2.bit_length() >= 2048):
                    continue
                print("YAY")
                break
    k += 1
    b = b0 + (k*p1*p2)

    n1_file = open("sol_1.2.5_factorsA.hex", "w")
    n1_file.write(hex(p1)[2:] + "\n")
    n1_file.write(hex(q1)[2:])
    n1_file.close()
    n2_file = open("sol_1.2.5_factorsB.hex", "w")
    n2_file.write(hex(p2)[2:] + "\n")
    n2_file.write(hex(q2)[2:])
    n2_file.close()
    privkey1, pubkey1 = certbldr.make_privkey(p1, q1)
    privkey2, pubkey2 = certbldr.make_privkey(p2, q2)
    certificateA = certbldr.make_cert('cli91', pubkey1, serial=serial_num)  
    certificateB = certbldr.make_cert('cli91', pubkey2, serial=serial_num)
    certA_file = open("sol_1.2.5_certA.cer", "wb")
    certA_file.write(certificateA.public_bytes(Encoding.DER))
    certA_file.close()
    certB_file = open("sol_1.2.5_certB.cer", "wb")
    certB_file.write(certificateB.public_bytes(Encoding.DER))
    certB_file.close()
    
if __name__ == "__main__":
    main()