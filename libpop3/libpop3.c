/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <pierrevr@mindgoo.be> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return. Pierre Vorhagen
 * ----------------------------------------------------------------------------
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>

#include "libpop3.h"

#define POP3_PORT 110
#define BUFSIZE 1024
#define RECV_BUFSIZE 4096

#define RETURN_ERROR()                                                         \
    do {                                                                       \
        fprintf(stderr, "%s (%d) : %s\n",__FILE__, __LINE__, strerror(errno)); \
        return -1;                                                             \
    } while(0)

/* private prototypes ======================================================= */

int /* socket */
pop3_connect(const char * pop3_server);

int /* success value (0/-1) */
pop3_send_command(
    int sock,
    const char * cmd,
    const char * data,
    char ** response
);

int /* success value (0/-1) */
pop3_pull_mail(
    int sock,
    int mail_count,
    char *** mails
);

/* public functions ========================================================= */

int libpop3_recv(int * mail_count, char *** mails,
    const char * pop3_server, const char * user, const char * pass)
{
    int ret;
    int sock;
    char buf[BUFSIZE];
    char * id;
    int i;
    char * response;

    /* connect to server */
    sock = pop3_connect(pop3_server);
    if (sock == -1) {
        return -1;
    }

    /* read welcome message */
    ret = recv(sock, buf, sizeof buf, 0);
    if (ret == -1) {
        RETURN_ERROR();
    }
    if (strstr(buf, "+OK") == NULL) {
        fprintf(stderr, "%s (%d) : Bad welcome message : %s", __FILE__, __LINE__, buf);
        return -1;
    }

#ifdef VERBOSE
    printf("<-- %s\n", buf);
#endif

    /* authenticate */
    ret = pop3_send_command(sock, "USER ", user, NULL);
    if (ret == -1) {
        return -1;
    }
    ret = pop3_send_command(sock, "PASS ", pass, NULL);
    if (ret == -1) {
        return -1;
    }

    /* check for mail */
    ret = pop3_send_command(sock, "STAT", NULL, &response);
    if (ret == -1) {
        return -1;
    }

    sscanf(response, "%*s %d ", mail_count);

    if (*mail_count > 0) { /* you've got mail (tm) */

        /* do magic with stat, retr and so forth */
        ret = pop3_pull_mail(sock, *mail_count, mails);
        if (ret == -1) {
            return -1;
        }

        /* delete messages from server */
        id = malloc(12);
        for (i = 1; i < (*mail_count + 1); i++) {
            sprintf(id, "%d", i);
            ret = pop3_send_command(sock, "DELE ", id, NULL);
            if (ret == -1) {
                return -1;
            }
        }
        free(id);
    }

    /* disconnect */
    ret = pop3_send_command(sock, "QUIT", NULL, NULL);
    if (ret == -1) {
        return -1;
    }
    ret = close(sock);
    if (ret == -1) {
        RETURN_ERROR();
    }

    return 0;
}

/* private functions ======================================================== */

int pop3_connect(const char * pop3_server)
{
    struct hostent * host;
    struct in_addr ip_address;
    int sock;
    struct sockaddr_in sin;
    int ret;

    host = gethostbyname(pop3_server);
    if (host == NULL) {
        RETURN_ERROR();
    }
    memcpy(&ip_address, host->h_addr_list[0], host->h_length);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        RETURN_ERROR();
    }

    memset(&sin, 0, sizeof sin);
    sin.sin_family = AF_INET;
    sin.sin_port = htons(POP3_PORT);
    sin.sin_addr = ip_address;
    ret = connect(sock, (struct sockaddr *)&sin, sizeof sin);
    if (ret == -1) {
        RETURN_ERROR();
    }

    return sock;
}

int pop3_send_command(int sock, const char * cmd, const char * data,
    char ** response)
{
    int ret;
    char * buf;
    char recvbuf[BUFSIZE];

    if (data != NULL) {
        buf = malloc(strlen(cmd) + strlen(data) + 3);
    }
    else {
        buf = malloc(strlen(cmd) + 3);
    }

    strcpy(buf, cmd);
    if (data != NULL) {
        strcat(buf, data);
    }
    strcat(buf, "\r\n");

#ifdef VERBOSE
    printf("--> %s\n", buf);
#endif

    ret = send(sock, buf, strlen(buf), 0);
    if (ret == -1) {
        RETURN_ERROR();
    }

    ret = recv(sock, recvbuf, sizeof recvbuf, 0);
    if (ret == -1) {
        RETURN_ERROR();
    }
    recvbuf[ret] = '\0';

    if (strstr(recvbuf, "+OK") == NULL) {
        fprintf(stderr, "%s (%d) : Bad pop3 response : %s", __FILE__, __LINE__, recvbuf);
        return -1;
    }

    if (response != NULL) {
        *response = malloc(strlen(recvbuf) + 1);
        strcpy(*response, recvbuf);
    }
#ifdef VERBOSE
    printf("<-- %s\n", recvbuf);
#endif

    return 0;
}

int pop3_pull_mail(int sock, int mail_count, char *** mails)
{
    int ret;
    int i;
    int msg_size;
    char buf[12];
    char * recvbuf;

    *mails = malloc(mail_count * sizeof (char *));

    for (i = 1; i < mail_count + 1; i++) {

        ret = sprintf(buf, "%d", i);
        if (ret < 0) {
            RETURN_ERROR();
        }

        ret = pop3_send_command(sock, "RETR ", buf, &recvbuf);
        if (ret == -1) {
            return -1;
        }

        sscanf(recvbuf, "%*s %d ", &msg_size);
        free(recvbuf);
        recvbuf = malloc(msg_size + 4);

        ret = recv(sock, recvbuf, msg_size + 3, 0);
        if (ret == -1) {
            RETURN_ERROR();
        }
        recvbuf[ret] = '\0';

        (*mails)[i - 1] = malloc(strlen(recvbuf));
        strcpy((*mails)[i - 1], recvbuf);

        free(recvbuf);
    }

    return 0;
}
