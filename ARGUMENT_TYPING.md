(This is tangential; attributes are not required at all for this)

It would be nice to bring typing to `argv`. `ls \*` ought not change behavior if there's a weird `-l` file.
The simplest way to pass typing is in an environment variable.
But it might be inherited by a child, so prefix the type info with some unique identifying information:

    ARG_TYPES="${argv[0]}:${getpid()}=?????;"

Verify `argv[0]`, `getpid()`, and that the number of the types matches `argc`. The `?`'s are a single character for each `argv`:

```
0 - arg0
? - unknown
. - filename
- - a switch or sub-command
; - optional end of types marker, deliminates region for future expansion
```

Note: The shell must not guess! If the user types `ls /`, the type is *always* `0?`.
A known type must only be given if the argument was provided via some UI, like tab completion, globbing, a drop down list, or explicit annotation.
(And eg editing a tab completion breaks the type, even if the text is the same. The shell must have mathematical certainty.)

The payoff for this doesn't seem very good. Most shells will probably send only `?` except for globs. A few commands might make good use of it.

<!--
<table>
<th>
    <td>If the user types</td><td>The argtype is</td><td></td>
</th>
<tr><td>`ls ./foo`</td>                        <td>`0?`</td><td>The shell must not guess what the user means.</td></tr>
<tr><td>`ls ./fo<TAB>`</td><td>`0-`</td>       <td>but only if the shell can PROVE that the name came from filename expansion.</td></tr>
<tr><td>`ls ./fo<TAB><Backspace>o`</td>        <td>`0?`</td><td>modifying the text of the argument removes the type</td></tr>
<tr><td>`ls \*`</td>                           <td>`0.`</td><td>assuming the shell performed the expansion by actually looking at the filesystem.</td></tr>
<tr><td>`ls -l`</td>                           <td>`0?`</td><td></td></tr>
</table>
-->




** See this thing from `man grep` **
       _N_GNU_nonoption_argv_flags_
              (Here N is grep's numeric process ID.)  If the ith character of this environment variable's value is 1, do not consider the ith operand of grep to be an option, even if it
              appears to be one.  A shell can put this variable in the environment for each command it runs, specifying which operands are the results of file  name  wildcard  expansion
              and therefore should not be treated as options.  This behavior is available only with the GNU C library, and only when POSIXLY_CORRECT is not set.

