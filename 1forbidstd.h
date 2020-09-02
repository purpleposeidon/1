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

#if L1_ALLOW_STDOUT == 0
#undef stdout
#undef puts
#define stdout         _Pragma("GCC error \"<1forbidstd.h>: usage of stdout\"")
#define STDOUT_FILENO  _Pragma("GCC error \"<1forbidstd.h>: usage of STDOUT_FIELNO\"")
#define printf(...)    _Pragma("GCC error \"<1forbidstd.h>: usage of printf\"")
#define vprintf(...)   _Pragma("GCC error \"<1forbidstd.h>: usage of vprintf\"")
#define putchar(_x)    _Pragma("GCC error \"<1forbidstd.h>: usage of putchar\"")
#define puts(_x)       _Pragma("GCC error \"<1forbidstd.h>: usage of puts\"")
#endif

#if L1_ALLOW_STDERR == 0
#undef stderr
#define stderr         _Pragma("GCC error \"<1forbidstd.h>: usage of stderr\"")
#define STDERR_FILENO  _Pragma("GCC error \"<1forbidstd.h>: usage of STDERR_FIELNO\"")
#define perror(_x)     _Pragma("GCC error \"<1forbidstd.h>: usage of perror\"")
#endif

#if L1_ALLOW_STDIN == 0
#undef stdin
#undef getc
#define stdin          _Pragma("GCC error \"<1forbidstd.h>: usage of stdin\"")
#define getc(_x)       _Pragma("GCC error \"<1forbidstd.h>: usage of getc\"")
#define getchar()      _Pragma("GCC error \"<1forbidstd.h>: usage of getchar\"")
#define scanf(...)     _Pragma("GCC error \"<1forbidstd.h>: usage of scanf\"")
#define vscanf(...)    _Pragma("GCC error \"<1forbidstd.h>: usage of vscanf\"")
#endif

#endif
