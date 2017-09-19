#!/usr/bin/env python
import sys, pbp, os.path
from Crypto.PublicKey import RSA
from functools import reduce
from math import *
from fractions import gcd

# def product(X):
#         return reduce(lambda x, y: x * y, X)

def producttree(X):
    result = [X]
    while len(X) > 1:
        X = [reduce(lambda x, y: x * y,X[i*2:(i+1)*2]) for i in range((len(X)+1)/2)]
        result.append(X)
    return result

# def remaindertree(n, T):
#     result = n
#     for t in list(reversed(T)):
#         result = [result[int(floor(i/2))] % t[i] for i in range(len(t))]
#     return result

def batchgcd_faster(ints):
        PT = producttree(ints)
        last = PT.pop()
        while PT:
                ints = PT.pop()
                last = [last[int(floor(i/2))] % ints[i]**2 for i in range(len(ints))]
        return [gcd(r/n, n) for r,n in zip(last, ints)]

def egcd(a, b):
    if a == 0:
        return (b, 0, 1)
    else:
        g, y, x = egcd(b % a, a)
        return (g, x - (b // a) * y, y)

def modinv(a, m):
    g, x, y = egcd(a, m)
    if g != 1:
        raise Exception('modular inverse does not exist')
    else:
        return x % m

def main():
    if len(sys.argv) != 2:
        print 'Wrong usage.'
        sys.exit(1)

    all_numbers = []
    with open('moduli.hex') as f:
        for line in f:
            all_numbers.append(int(line, 16))

    if not os.path.isfile('gcd_list.txt'):
        gcds = batchgcd_faster(all_numbers)
        with open('gcd_list.txt', 'w') as f:
            for num in gcds:
                f.write("%d\n" % num)
    else:
        with open('gcd_list.txt') as f:
            gcds = [int(line) for line in f]

    with open(sys.argv[1]) as f:
        ciphertext = f.read()

    e = 65537L
    for i in range(len(gcds)):
        if gcds[i] != 1:
            p = gcds[i]
            q = all_numbers[i]/p
            d = modinv(e, (p-1)*(q-1))

            try:
                key = RSA.construct((p*q, e, d))
                plaintext = pbp.decrypt(key, ciphertext)
                print plaintext
            except ValueError:
                continue

if __name__ == "__main__":
    main()
