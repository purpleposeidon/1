#ifndef __L1
#define __L1

// lib1 is a protocol for improving the Unix coreutils, pipelines, and terminals.
// It is backwards compatible with the existing ecosystem.

#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>


struct l1_atr {
    int fd;
    int protocol_version;
    int status;
};
#define L1_ATR_STATUS_UNKNOWN  0
#define L1_ATR_STATUS_ATR      1
#define L1_ATR_STATUS_STD      2

struct l1_atr l1_connect_reader(int fd);
struct l1_atr l1_connect_writer(int fd);
struct l1_atr l1_conect_std(int fd);

struct l1_atr l1_atrin;
struct l1_atr l1_atrout;

void l1_init();
void l1_init_lib();
void l1_init_atr();

struct l1_packet {
    char *msg;
    size_t len;
};

ssize_t l1_write(struct l1_atr *atr, struct l1_packet *packet);

void l1_printf(char t, const char *fmt, ...);

struct l1_packet l1_vpacketf(char t, const char *fmt, va_list ap);
struct l1_packet l1_packetf(char t, const char *fmt, ...);
void l1_vfwritef(struct l1_atr *atr, char t, const char *fmt, va_list ap);
void l1_fwritef(struct l1_atr *atr, char t, const char *fmt, ...);

struct l1_packet l1_clone_packet(struct l1_packet *p);
void l1_drop_packet(struct l1_packet p);

char l1_packet_type(const struct l1_packet *p);
char *l1_packet_msg(const struct l1_packet *p);
#define L1_PUSH           '('
#define L1_POP            ')'
#define L1_FIELD_NAME     '$'
#define L1_FIELD_TYPE     ':'
#define L1_FIELD_VALUE    '='
#define L1_FIELD_TEXT     '>'
#define L1_FORMAT         '-'
#define L1_COMMENT        '_'
#define L1_LABEL          '*'


typedef signed long long l1_entity; // FIXME: how to i64 I know you can do this how does it done.

void l1_push(l1_entity entity);
void l1_pop(l1_entity entity);
void l1_field_name(const char *field_name);
void l1_field_type(const char *field_type);
void l1_field_value(const char *field_value);
void l1_field_text(const char *field_text);
void l1_format(const char *formatting);
void l1_comment(const char *comment);

// so ls might output something like...
//     [     ][(1][$filename][:uri][=file:///etc/passwd][␤passwd][)1][ ␤]
// which raw is
//          (1$filename:uri=file:///etc/passwd
//     passwd)1 
// which is 0rendered as
//         passwd␤
//
// Eg the output of
//     $ date --date='@1147483647'
// is
//     Fri May 12 18:27:27 PDT 2006
// annotated as
//     [(1][:utctimestamp][=1147483647]
//         [$day][>Fri][  ]
//         [$month][>May][   ]
//         [$day][>12][  ]
//         [$hour][>18][ :]
//         [$minute][>27][ :]
//         [$second][>27][ :]
//         [$timezone][>PDT][  ]
//         [$year][>2006]
//     [)1]

ssize_t l1_read(struct l1_atr *atr, struct l1_packet *dst);



ssize_t l1_putchar(char t, char c);
ssize_t l1_puts(char t, const char *msg);

int l1_flush(struct l1_atr *atr);

// FIXME: Only if `register_printf_specifier` is available
//#include <features.h>
//#if defined _GNU_SOURCE && defined __GLIBC__

#define L1_PFMT(x)        "%-1m" x
#define L1_P              L1_PFMT("%c")
#define L1_PPUSH          L1_PFMT("(")
#define L1_PPOP           L1_PFMT(")")
#define L1_PFIELD_NAME    L1_PFMT("$")
#define L1_PFIELD_TYPE    L1_PFMT(":")
#define L1_PFIELD_VALUE   L1_PFMT("=")
#define L1_PFIELD_TEXT    L1_PFMT(">")
#define L1_PFORMAT        L1_PFMT("-")
#define L1_PCOMMENT       L1_PFMT("_")
#define L1_PLABEL         L1_PFMT("*")
// printf with packet separation formatting codes.
// Use the L1_P* class of macros to indicate packet separation boundaries.
// The format string must start with an L1_P*.
//
// The L1_P format string macro can be used to set the packet's type dynamically;it takes the packet type as a format argument.
//
// Example:
//   l1_printp(
//      L1_PFORMAT "Hello, %s!"
//      L1_P       "Uhm, %s to meet you",
//      "world",
//      L1_FIELD_VALUE, "cheesed"
//   );
//
// %n probably doesn't have the greatest definition. Right now it includes the packet header; it would make much more sense for it to not.
// Perhaps it will in the future.
// You can use the L1_ADJUST_PERCENT_N macro to fix this.
//
//   int n;
//   l1_printp(
//       L1_PFORMAT "Hello world"
//       L1_PFORMAT "I'm very worried about padding: %n",
//       &n
//   );
//   n = adjust_padding(n,
//         1 // packet 'Hello...'
//       + 1 // packet 'I'm very...'
//   );
//
//
// Note that _GNU_SOURCE is required as this uses glibc's printf formatter registration.
// "%-1m" is the format code used; it is unlikely that this will be a problem for you.
ssize_t l1_printp(const char *fmt, ...);

#define L1_ADJUST_PERCENT_N(number_of_bytes, prior_packets) (n - prior_packets)

//#endif


#endif
