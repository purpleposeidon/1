#!/usr/bin/python3


import os, sys


print("Hi, I'm check.py", file=sys.stderr)
for f in list(os.listdir('/proc/self/fd')):
    try: l = os.readlink('/proc/self/fd/' + f)
    except: l = '?'
    print(f, l, file=sys.stderr)

