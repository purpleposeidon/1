#!/usr/bin/python3


x = [int(_) for _ in [x for x in open("/tmp/b") if '//DIRED//' in x][0].split()[1:]]
print("...")
f = open("/tmp/b").read()
print(x)
x = iter(x)

try:
    while True:
        a = next(x)
        b = next(x)
        print(f[a:b])
except StopIteration: pass

tk = open("/tmp/b").read()
print('dired @', tk.index("//DIRED//"))
