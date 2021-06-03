#!/usr/bin/python3

import time

while True:
    a = open("/tmp/b").read()
    print(len(a), repr(a))
    time.sleep(1)
