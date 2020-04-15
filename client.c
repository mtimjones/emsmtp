/*
 * client.c
 *
 * Tiny mail client functions.
 *
 */

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/un.h>
#include <unistd.h>
#include "client.h"

const char *hello_msg={"HELO mydomain.com\n"};
const char *data_msg={"DATA\n"};
const char *mail_from={SOURCE_EMAIL_ADDRESS};
const char *endmail={"\n.\n"};
const char *quit_msg={"QUIT\n"};

/*
 * Grab the response code from the string.
 */
int codescan(char *line, char *str, int len)
{
  int i;

  for (i = 0 ; i < strlen(line) - len ; i++) {
    if (line[i] == str[0]) {
      if (!strncmp(line, str, len)) return(i);
    }
  }

  return(-1);
}


/*
 * mailSend performs a simple form of the client side of SMTP.  A mail 
 * structure is passed that contains the email to send.
 */
int mailSend(struct mailHeader *mail)
{
  int connfd, ret;
  struct sockaddr_in servaddr;
  char mailServer[129], line[257];

  strcpy(mailServer, SMTPGATEWAY);

  connfd = socket(AF_INET, SOCK_STREAM, 0);

  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(25);

  /* Resolve the mail server name if not a raw IP address */
  if (!isdigit(mailServer[0])) {
    struct hostent *hptr = (struct hostent *)gethostbyname(mailServer);
    if (hptr == NULL) {
      printf("Bad hptr...\n");
      return(-1);
    } else {
      struct in_addr **addrs;
      addrs = (struct in_addr **)hptr->h_addr_list;
      memcpy(&servaddr.sin_addr, 
      *addrs, sizeof(struct in_addr));
    }
  } else {
    servaddr.sin_addr.s_addr = inet_addr(mailServer);
  }

  connect(connfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

  do {
    /* Look for initial salutation */
    ret = read(connfd, line, sizeof(line)-1); line[ret] = 0;
    if (codescan(line, "220", 3) == -1) break;

    /* Send HELO and await response */
    ret = write(connfd, hello_msg, strlen(hello_msg));
    if (ret != strlen(hello_msg)) break;
    ret = read(connfd, line, sizeof(line)-1); line[ret] = 0;
    if (codescan(line, "250", 3) == -1) break;

    /* Send MAIL FROM and await response */
    sprintf(line, "MAIL FROM: %s\n", mail_from);
    ret = write(connfd, line, strlen(line));
    if (ret != strlen(line)) break;
    ret = read(connfd, line, sizeof(line)-1); line[ret] = 0;
    if (codescan(line, "250", 3) == -1) break;

    /* Send RCPT TO and await response */
    sprintf(line, "RCPT TO: %s\n", mail->mailRcpt);
    ret = write(connfd, line, strlen(line));
    if (ret != strlen(line)) break;
    ret = read(connfd, line, sizeof(line)-1); line[ret] = 0;
    if (codescan(line, "250", 3) == -1) break;


    /* Send DATA and await response */
    ret = write(connfd, data_msg, strlen(data_msg));
    if (ret != strlen(data_msg)) break;
    ret = read(connfd, line, sizeof(line)-1); line[ret] = 0;
    if (codescan(line, "354", 3) == -1) break;

    /* Send out the header first */
    sprintf(line, "From: %s\n", mail_from);
    write(connfd, line, strlen(line));

    sprintf(line, "To: %s\n", mail->mailRcpt);
    write(connfd, line, strlen(line));

    sprintf(line, "Subject: %s\n", mail->subject);
    write(connfd, line, strlen(line));

    if (mail->contentType[0] != 0) {
      sprintf(line, "Content-Type: %s\n", mail->contentType);
      write(connfd, line, strlen(line));
    }

    strcpy(mail->specialHeaders, "Priority: Urgent\n");

    if (mail->specialHeaders[0] != 0) {
      write(connfd, mail->specialHeaders, strlen(mail->specialHeaders));
    }

    write(connfd, mail->contents, strlen(mail->contents));

    /* Send mail-end and await response */
    ret = write(connfd, endmail, strlen(endmail));
    if (ret != strlen(endmail)) break;
    ret = read(connfd, line, sizeof(line)-1); line[ret] = 0;
    if (codescan(line, "250", 3) == -1) break;

  } while (0);

  /* Send "QUIT" and await response */
  ret = write(connfd, quit_msg, strlen(quit_msg));

  close(connfd);

  return(0);
}
