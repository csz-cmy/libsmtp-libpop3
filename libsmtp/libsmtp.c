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

#include "libsmtp.h"

#define SMTP_PORT 25
#define BUFSIZE 1024
#define RET_BUFSIZE 1024
#define DATE_BUFSIZE 40

#define RETURN_ERROR()                                                         \
    do {                                                                       \
        fprintf(stderr, "%s (%d) : %s\n",__FILE__, __LINE__, strerror(errno)); \
        return -1;                                                             \
    } while(0)

/* private prototypes ======================================================= */

int /* socket */
smtp_connect(const char * smtp_server);

int /* success value (0/-1) */
smtp_send_command(
    int sock,
    const char * command,
    const char * data,
    int return_code
);

int /* success value (0/-1) */
smtp_send_data(
    int sock,
    const libsmtp_msg_t * msg
);

/* public functions ========================================================= */

int libsmtp_send(const char * smtp_server, const libsmtp_msg_t * msg)
{
    int ret;
    int sock;
    char buf[BUFSIZE];

    /* connect to smtp server */
    sock = smtp_connect(smtp_server);
    if (sock == -1) {
        return -1;
    }

    /* read welcome message */
    ret = recv(sock, buf, sizeof buf, 0);
    if (ret == -1) {
        RETURN_ERROR();
    }
    if (strstr(buf, "220 ") == NULL) {
        fprintf(stderr, "%s (%d) : Bad welcome message : %s", __FILE__, __LINE__, buf);
        return -1;
    }

#ifdef VERBOSE
    printf("<-- %s\n", buf);
#endif

    /* send initiation commands */
    ret = smtp_send_command(sock, "MAIL FROM: ", msg->from, 250);
    if (ret == -1) {
        return -1;
    }
    ret = smtp_send_command(sock, "RCPT TO: ", msg->to, 250);
    if (ret == -1) {
        return -1;
    }

    /* send data */
    ret = smtp_send_command(sock, "DATA", NULL, 354);
    if (ret == -1) {
        return -1;
    }
    ret = smtp_send_data(sock, msg);
    if (ret == -1) {
        return -1;
    }

    /* disconnect */
    ret = smtp_send_command(sock, "QUIT", NULL, 221);
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

int smtp_connect(const char * smtp_server)
{
    struct hostent * host;
    struct in_addr ip_address;
    int sock;
    struct sockaddr_in sin;
    int ret;

    host = gethostbyname(smtp_server);
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
    sin.sin_port = htons(SMTP_PORT);
    sin.sin_addr = ip_address;
    ret = connect(sock, (struct sockaddr *)&sin, sizeof sin);
    if (ret == -1) {
        RETURN_ERROR();
    }

    return sock;
}

int smtp_send_command(int sock, const char * cmd, const char * data,
    int return_code)
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

    free(buf);
    buf = malloc(BUFSIZE);
    ret = sprintf(buf, "%d ", return_code);
    if (ret < 0) {
        RETURN_ERROR();
    }

    if (strstr(recvbuf, buf) == NULL) {
        fprintf(stderr, "%s (%d) : Bad smtp response : %s", __FILE__, __LINE__, recvbuf);
        return -1;
    }

#ifdef VERBOSE
    printf("<-- %s\n", recvbuf);
#endif

    free(buf);

    return 0;
}

int smtp_send_data(int sock, const libsmtp_msg_t * msg)
{
    int ret;
    char * buf;
    char date[DATE_BUFSIZE];
    char recvbuf[BUFSIZE];
    short subject_defined = 0;
    time_t send_time = time(NULL);

    if (msg->subject != NULL) {
        subject_defined = 1;
    }

    if (subject_defined) {
        buf = malloc(strlen(msg->to) + strlen(msg->from) + strlen(msg->data)
            + DATE_BUFSIZE + 100 + strlen(msg->subject));
    }
    else {
        buf = malloc(strlen(msg->to) + strlen(msg->from) + strlen(msg->data)
            + DATE_BUFSIZE + 100);
    }

    strftime(date, DATE_BUFSIZE, "%a, %d %b %Y  %H:%M:%S +0000", gmtime(&send_time));

    strcpy(buf, "From: ");
    strcat(buf, msg->from);
    strcat(buf, "\r\nTo: ");
    strcat(buf, msg->to);
    strcat(buf, "\r\nDate: ");
    strcat(buf, date);
    if (subject_defined) {
        strcat(buf, "\r\nSubject: ");
        strcat(buf, msg->subject);
    }
    strcat(buf, "\r\n\r\n");
    strcat(buf, msg->data);
    strcat(buf, "\r\n.\r\n");

#ifdef VERBOSE
    printf("--> \n%s\n", buf);
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

#ifdef VERBOSE
    printf("<-- %s\n", recvbuf);
#endif

    free(buf);

    buf = malloc(12);
    ret = sprintf(buf, "%d ", 250);
    if (ret < 0) {
        RETURN_ERROR();
    }

    if (strstr(recvbuf, buf) == NULL) {
        fprintf(stderr, "%s (%d) : Bad smtp response : %s", __FILE__, __LINE__, recvbuf);
        return -1;
    }

    free(buf);

    return 0;
}
