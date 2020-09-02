#!/usr/bin/python3

import os, sys, socket, time


def s2l(x):
    if type(x) == str:
        if not ' ' in x: return [x, x]
        x = x.split(' ')
        return [x[0]] + x
    return x

def run(a, b):
    a = s2l(a)
    b = s2l(b)
    print("+ {0}  ⊢  {1}".format(' '.join(a[1:]), ' '.join(b[1:])))


    pipe_rw = socket.socketpair(family=socket.AF_UNIX, type=socket.SOCK_SEQPACKET, proto=0)
    pipe_r = pipe_rw[0].fileno()
    pipe_w = pipe_rw[1].fileno() # gc = close



    STDIN_FILENO = 0
    STDOUT_FILENO = 1
    STDERR_FILENO = 2

    a_pid = os.fork()
    if a_pid == 0:
        os.dup2(pipe_w, STDOUT_FILENO)
        os.close(pipe_r)
        os.close(pipe_w)
        os.environ["LIB1"] = f"protocol_version:0"
        os.execl(*a)
        raise Exception

    b_pid = os.fork()
    if b_pid == 0:
        os.dup2(pipe_r, STDIN_FILENO)
        os.close(pipe_r)
        os.close(pipe_w)
        os.environ["LIB1"] = f"protocol_version:0"
        os.execl(*b)
        raise Exception


    os.close(pipe_r)
    os.close(pipe_w)

    a = os.wait()
    b = os.wait()



#a = ['/bin/cat', 'cat']
#b = ['/bin/sed', 'sed', 's/[aeiou]/_/g']

os.system("make")
run("./hello", "./deatr")
os.system("cd coreutils; make src/ls")
run("./coreutils/src/ls", "./deatr")
run("./coreutils/src/ls -l", "./deatr")
run("./coreutils/src/ls -im", "./deatr")
# FIXME: valgrind
