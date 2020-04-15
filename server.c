/*
 * server.c
 *
 * Tiny mail server functions.
 *
 */

#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <sys/un.h>
#include <unistd.h>

const char *salutation={"220 Tiny Mail Transport\n"};
const char *conclose={"221 TMT closing connection.\n"};
const char *goahead={"250 OK\n"};
const char *gimmesome={"354 Gimme!\n"};
const char *gotit={"250 Message accepted for delivery.\n"};
const char *closeit={"221 TMT closing transmission channel.\n"};

#define MAX_EMAIL_SIZE	4096

static char mail[MAX_EMAIL_SIZE];

/*
 * Signal handler for SIGPIPE
 */
void ouch ( int sigNum )
{
  // Avoid EPIPE crash...
}


/*
 * Pull the email address from the 'MAIL FROM'/'RCPT TO' headers.
 */
void parseAddress(char *src, char *dest, int len)
{
  int i, j=0;

  for (i = 0 ; i < len ; i++) {
    if (src[i] != ':' && src[i] != ' ' && src[i] != '<' && src[i] != '>') {
      if ((src[i] == 0x0d) || (src[i] == 0x0a)) break;
      dest[j++] = src[i];
    }
  }
  dest[j] = 0;

  return;
}


/*
 * Close the connection by sending the close message to the client.
 */
void closeConnection(int fd)
{
  write(fd, conclose,  strlen(conclose));
}


/*
 * We've received a socket connect to the SMTP server port.  This function
 * will communicate with the client using a very simple form of the SMTP
 * protocol.
 */
void mailReceive(int fd)
{
  int bufIdx = 0, state = 0, stop = 0, i, len;
  char buffer[257]={0};
  char sender[257], recipient[257];

  int mailParse(char *, int, char *);

  /* Indicate ready to go and handle premature close or port-scan */
  if (write(fd, salutation, strlen(salutation)) <= 0) {
    return;
  }

  /* Wait for salutation response... */
  len = read(fd, buffer, 255);  buffer[len] = 0;

  if ((strncmp(buffer, "HELO", 4)) && (strncmp(buffer, "EHLO", 4))) {
    closeConnection(fd);
    return;
  }

  /* Wait for Mail From: */
  write(fd, goahead, strlen(goahead));
  len = read(fd, buffer, 255);  buffer[len] = 0;
  if (strncmp(buffer, "MAIL FROM", 9)) { closeConnection(fd); return; }
  parseAddress(&buffer[9], sender, 256);
  if (sender[0] == 0)  { closeConnection(fd); return; }

  /* Wait for Rcpt To: */
  write(fd, goahead, strlen(goahead));
  len = read(fd, buffer, 255);  buffer[len] = 0;
  if (strncmp(buffer, "RCPT TO", 7)) { closeConnection(fd); return; }
  parseAddress(&buffer[7], recipient, 256);
  if (recipient[0] == 0)  { closeConnection(fd); return; }

  // Wait for DATA
  write(fd, goahead, strlen(goahead));
  len = read(fd, buffer, 255);  buffer[len] = 0;
  if (strncmp(buffer, "DATA", 4)) { closeConnection(fd); return; }

  write(fd, gimmesome, strlen(gimmesome));

  while (!stop) {
    len = read(fd, buffer, 80);

    if (bufIdx+len > MAX_EMAIL_SIZE) { closeConnection(fd); return; }

    if (len <= 0) { closeConnection(fd); return; }
    else {
      memcpy(&mail[bufIdx], buffer, len);
      bufIdx += len;
    }

    /* Look for the terminating <CR><LF>.<CR><LF> */
    for (i = 0 ; i < len ; i++) {
      if      ((state == 0) && (buffer[i] == 0x0d)) state = 1;
      else if ((state == 1) && (buffer[i] == 0x0a)) state = 2;
      else if ((state == 2) && (buffer[i] == 0x0d)) state = 1;
      else if ((state == 2) && (buffer[i] == '.'))  state = 3;
      else if ((state == 3) && (buffer[i] == 0x0d)) state = 4;
      else if ((state == 4) && (buffer[i] == 0x0a)) { stop = 1; break; }
      else state = 0;
    }

  }

  write(fd, gotit, strlen(gotit));

  /* Wait for QUIT */
  for (i = 0 ; i < 10 ; i++) {

    len = read(fd, buffer, 512);  buffer[len] = 0;

    if (strncmp(buffer, "QUIT", 4)) { 
      closeConnection(fd); return; 
    } else break;

  }

  mailParse(mail, bufIdx, sender);

  write(fd, closeit, strlen(closeit));

  return;
}


/*
 * Our main function to create the server socket and await requests.
 */
int main(int argc, char **argv)
{

  int listenfd, connfd;
  socklen_t clilen;
  struct sockaddr_in cliaddr, servaddr;

  (void)signal(SIGPIPE, ouch);

  listenfd = socket(AF_INET, SOCK_STREAM, 0);

  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(25);

  bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

  listen(listenfd, 1);

  for ( ; ; ) {

    clilen = sizeof(cliaddr);

    connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
    if (connfd <= 0) break;

    mailReceive(connfd);

    close(connfd);

  }

  close(listenfd);
  return(0);
}

