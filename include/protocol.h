#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

enum msg_types {
    OK = 0x00,
    LOGIN = 0x10,
    LOGOUT = 0x11,
    EUSRLGDIN = 0x1A,
    EWRNGPWD = 0x1B,
    ANCREATE = 0x20,
    ANCLOSED = 0x22,
    ANLIST = 0x23,
    ANWATCH = 0x24,
    ANLEAVE = 0x25,
    ANBID = 0x26,
    ANUPDATE = 0x27,
    EANFULL = 0x2B,
    EANNOTFOUND,
    EANDENIED,
    EBIDLOW,
    EINVALIDARG,
    USRLIST = 0x32,
    USRWINS = 0x33,
    USRSALES = 0x34,
    USRBLNC = 0x35,
    ESERV = 0xff
};

// This is the struct describes the header of the PETR protocol messages
typedef struct {
    uint32_t msg_len; // this should include the null terminator
    uint8_t msg_type;
} petr_header;

int rd_msgheader(int socket_fd, petr_header *h);
int wr_msg(int socket_fd, petr_header *h, char *msgbuf);

#endif
