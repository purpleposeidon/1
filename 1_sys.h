#ifndef __1_SYS_H
#define __1_SYS_H

#define L1_DEV_PATH            "/dev/1"

#include <asm-generic/ioctl.h>
// [L1_IOCTL_DEFS]
#define L1_IOCTL_WRAP          _IOW('1', 0, int)
#define L1_IOCTL_UNWRAP        _IO ('1', 1)
#define L1_STEM_VAR            "LIB1_STEM_FD"
// #define L1_IOCTL_MARK_STEM     _IOW('1', 2, int)
// #define L1_IOCTL_FIND_STEM     _IO ('1', 3)

#define L1_STEM_IOV_PAYLOAD    '1'


struct __attribute__((packed, aligned(1))) l1_packet {
    unsigned short msg_len; // Length of the payload, excluding the header.
    unsigned char version; // Length of the rest of the header. Also does versioning. Always >= 1, kernel needs flags.
#define L1_P_HEADER_PRELUDE_LEN (sizeof(unsigned short) + sizeof(unsigned char))
#define L1_P_VERSION (sizeof(struct l1_packet) - L1_P_HEADER_PRELUDE_LEN - sizeof(unsigned char*))
#define L1_P_HEADER_LEN (sizeof(struct l1_packet) - sizeof(char*))
    char flags; // This must always be first.
#define L1_PF_STD_VISIBLE 0x1
#define L1_PF_SYNTHETIC   0x2 // FIXME: If synthetic is a type, then we don't need a flag.
#define L1_PF_INCOMPLETE  0x4
// mayb L1_PF_BINARY      0x8 ?? Would be good for entity IDs, and also not showing noisey blobs in an inspector. (But if you're inspecting, it's not supposed to be visible, so maybe always escaping such data would be reasonable?)
    char type; // How this packet should be interpreted. See other [EACH_L1_HEADER_TYPE]
    // FIXME: the 'visible' flag means that stuff needs to be worked out.
#define L1_PUSH           '('
#define L1_POP            ')'
#define L1_FIELD_NAME     '$'
#define L1_FIELD_TYPE     ':'
#define L1_FIELD_VALUE    '='
#define L1_FIELD_TEXT     '>'
#define L1_FORMAT         '-'
#define L1_COMMENT        '_'
#define L1_LABEL          '*'
// #define L1_MAGIC      '1'  // always write this out in case stdout is being saved to a file? OTOH, whose responsibility is that really?
// "cmd > file" can't work even a little bit. It could be outputting a png. So files must be std.
// Files should probably be done w/ a special command that writes them in a human-readable format?
#define L1_SYNTHETIC      '0'
    char *msg; // Data follows header when serialized.
    // FIXME: void *msg ???
};


#endif /* __1_SYS_H */
