# lib1

Yet another attempt at making the shell nicer. This one intends to be backwards compatible.
Existing coreutils & friends will be modified to become more useful, and your existing shell scripts will continue to work.

Some things that could happen in a fully fleshed out lib1 ecosystem:
1. `ssh other_host -- ls | grep log`, right click a file name, `Open with...`
2. Running `some_new_program --help` teaches your shell how to tab-complete that command.
3. Spaces in filenames are no longer a problem.
4. TUI programs can get the same events as GUI events. Janky escape sequences are no more.

Instead of a flat string of bytes down a unix pipe, we use a bi-directional unix socket of type `SOCK_SEQPACKET`.
(This is like UDP, except ordered & reliable.)
The packets give delimintation, and the first byte gives the packet's type.
There are two packet types: printed and non-printed.
Filtering out the non-printed packets produces output that is identical to the standard output. This is where the backwards compatibility comes from.


# Terminology
lib1 - This scheme is called lib1 because many terminal programs have a `-0` option, to take input separated by nuls instead of by newlines.
We can do better than this: death to inband signalling!
STD - The existing pipeline programs write to STDOUT and read from STDIN, so they are STD programs.
ATR - lib1 programs write to ATROUT and read from ATRIN, so they are ATR programs.
ls1 - If I write a program name as ending in 1, I mean that it uses ATR rather than STD. No programs are actually renamed.
ls0 - Similarly, 0 is some STD program as it exists already.
`$head | $tail` - `$head` and `$tail` are the writing & reading parts of a pipeline.








# Rendezvous: The Problem
But this scheme has a very difficult problem.
If I run `ls1 | cat0`, what should happen? `ls1` must somehow figure out that it is talking to a `STD` program, so that it will act like `ls0`. How can it do that?
Here is the sketch of the ideal protocol:

`$head` and `$tail` each individually signify their commitment to the protocol. This blocks. If both sides make this commitment, the rendezvous succeeds.
But if `$head` writes something without signalling commitment, or if `$tail` tries to read without making the commitment, then the rendezvous fails and the format is STD.

There are lots of things that make this actually kind of difficult.

0. There's no way to annotate a binary and also a shell script.
1. Programs could be slow, or the system could be under heavy load.
2. [The Two Generals' Problem](https://en.wikipedia.org/wiki/Two_Generals%27_Problem). One end of the pipe can't rely on the other end to give a signal; essentially any signalling mechanism must include a built-in acknowledgement that it was received.

These are basic issues. It gets worse:

1. What does `cat` see in `( ls0 /a/; ls1 /b/ ) | cat`? What if it's `cat0`, and what if it's `cat1`?
2. What happens if an `ATR` program `exec`s a `STD` program, or vice-versa?


## Level 0: Timing
## Level 1: `LD_PRELOAD`
## Level 2: The Kernel
