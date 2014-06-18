#ifndef __BACKPORT_SOCKET_H
#define __BACKPORT_SOCKET_H
#include_next <linux/socket.h>

#ifndef SOL_NFC
/*
 * backport SOL_NFC -- see commit:
 * NFC: llcp: Implement socket options
 */
#define SOL_NFC		280
#endif

#endif /* __BACKPORT_SOCKET_H */
