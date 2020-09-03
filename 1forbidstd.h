#ifndef __L1_FORBID_STD
#define __L1_FORBID_STD

// Convert stdio usage into a compile error.
// This header is controlled by the macros
//   L1_ALLOW_STDOUT  (which defaults to false)
//   L1_ALLOW_STDERR  (which defaults to true)
//   L1_ALLOW_STDIN   (which defaults to false)

#ifndef L1_ALLOW_STDOUT
#define L1_ALLOW_STDOUT 0
#endif

#ifndef L1_ALLOW_STDERR
#define L1_ALLOW_STDERR 1
#endif

#ifndef L1_ALLOW_STDIN
#define L1_ALLOW_STDIN 0
#endif


// Functions like fprintf that take a FILE *stream are okay; instead deny access to stdout.

#define L1_FORBID(x)   do { _Pragma(x); } while (0)

#if L1_ALLOW_STDOUT == 0
#undef stdout
#undef puts
#define stdout         L1_FORBID("GCC error \"<1forbidstd.h>: usage of stdout\"")
#define STDOUT_FILENO  L1_FORBID("GCC error \"<1forbidstd.h>: usage of STDOUT_FIELNO\"")
#define printf(...)    L1_FORBID("GCC error \"<1forbidstd.h>: usage of printf\"")
#define vprintf(...)   L1_FORBID("GCC error \"<1forbidstd.h>: usage of vprintf\"")
#define putchar(_x)    L1_FORBID("GCC error \"<1forbidstd.h>: usage of putchar\"")
#define puts(_x)       L1_FORBID("GCC error \"<1forbidstd.h>: usage of puts\"")
#endif

#if L1_ALLOW_STDERR == 0
#undef stderr
#define stderr         L1_FORBID("GCC error \"<1forbidstd.h>: usage of stderr\"")
#define STDERR_FILENO  L1_FORBID("GCC error \"<1forbidstd.h>: usage of STDERR_FIELNO\"")
#define perror(_x)     L1_FORBID("GCC error \"<1forbidstd.h>: usage of perror\"")
#endif

#if L1_ALLOW_STDIN == 0
#undef stdin
#undef getc
#define stdin          L1_FORBID("GCC error \"<1forbidstd.h>: usage of stdin\"")
#define getc(_x)       L1_FORBID("GCC error \"<1forbidstd.h>: usage of getc\"")
#define getchar()      L1_FORBID("GCC error \"<1forbidstd.h>: usage of getchar\"")
#define scanf(...)     L1_FORBID("GCC error \"<1forbidstd.h>: usage of scanf\"")
#define vscanf(...)    L1_FORBID("GCC error \"<1forbidstd.h>: usage of vscanf\"")
#endif

#endif
