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

struct l1_atr l1_atrin;
struct l1_atr l1_atrout;
struct l1_atr l1_atrerr;

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

#endif
