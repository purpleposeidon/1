#!/usr/bin/python3

import os

p = os.pipe()
os.fork()
os.system("ls -l --color /proc/%s/fd" % os.getpid())
