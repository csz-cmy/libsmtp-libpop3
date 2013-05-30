/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <pierrevr@mindgoo.be> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return. Pierre Vorhagen
 * ----------------------------------------------------------------------------
 */

#ifndef LIBSMTP_H
#define LIBSMTP_H

typedef struct libsmtp_msg {
    char * to;
    char * from;
    char * subject;
    char * data;
} libsmtp_msg_t;

int /* success value (0/-1) */
libsmtp_send(
    const char * smtp_server,
    const libsmtp_msg_t * msg
);

#endif
