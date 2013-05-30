/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <pierrevr@mindgoo.be> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return. Pierre Vorhagen
 * ----------------------------------------------------------------------------
 */

#ifndef LIBPOP3_H
#define LIBPOP3_H

int /* success value (0/-1) */
libpop3_recv(
    int * mail_count,
    char *** mails,
    const char * pop3_server,
    const char * user,
    const char * pass
);

#endif
