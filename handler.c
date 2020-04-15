/*
 * handler.c
 *
 * Incoming mail parse functions and automatic response functions.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "client.h"

/*
 * Parse the received email and call the particular handler based upon the
 * request.
 */
int mailParse(char *mbuf, int len, char *sender)
{
  int i = 0, index=0;
  char subject[MAX_SUBJECT];

  int sendResponse(char *);
  int sendSensorData(char *);
  int processCommand(char *, char *);

  bzero(&subject, MAX_SUBJECT);

  /* Pull the subject line as part of the command. */
  for (i = 0 ; i < len ; i++) {
    if (mbuf[i] == 'S') {
      if (!strncmp(&mbuf[i], "Subject:", 8)) {
        for (i+= 9; ((mbuf[i] != 0x0d) && (mbuf[i] != 0x0a)) ; i++) {
          subject[index++] = mbuf[i];
          if (index == MAX_SUBJECT-1) break;
        }
        subject[index] = 0;
        break;
      }
    }
  }

  /* Figure out what the sender wanted and reply accordingly */
  if        (!strncmp(subject, "PING", 4)) {
    printf("Received PING command\n");
    sendResponse(sender);
  } else if (!strncmp(subject, "SENSOR", 6)) {
    printf("Received SENSOR command\n");
    sendSensorData(sender);
  } else if (!strncmp(subject, "RESET", 5)) {
    printf("Received RESET command\n");
    processCommand(sender, subject);
  } else {
    /* Don't respond if we didn't receive a valid request. */
    printf("No handler available for [%s]\n", subject);
  }

  return(0);
}


/*
 * Send back a ping response 
 */
int sendResponse(char *sender)
{
  struct mailHeader mail;

  bzero(&mail, sizeof(struct mailHeader));

  strcpy(mail.subject, "PONG");

  strcpy(mail.mailRcpt, sender);
  strcpy(mail.contentType, "text/plain");
  sprintf(mail.contents, "\nThe unit is operational.\n\n");

  mailSend(&mail);

  return(0);
}


/*
 * Send back some simulated sensor data in HTML format. 
 */
int sendSensorData(char *sender)
{
  #define MAX_SENSORS	3
  struct mailHeader mail;
  int dummySensorData[MAX_SENSORS]={28, 11, 19};
  int i;

  bzero(&mail, sizeof(struct mailHeader));

  strcpy(mail.subject, "Sensor Data Response");

  strcpy(mail.mailRcpt, sender);
  strcpy(mail.contentType, "text/html");

  strcat(mail.contents, "<HTML><HEAD><TITLE>Sensor Data</TITLE></HEAD>");
  strcat(mail.contents, "<BODY><CENTER>");
  strcat(mail.contents, "<H2>Current Readings are:</H2><P><P>");

  strcat(mail.contents, "<TABLE BORDER><CAPTION ALIGN=BOTTOM>");
  strcat(mail.contents, "<I>Sensor Data</I>");
  strcat(mail.contents, "</CAPTION>");
  strcat(mail.contents, "<TR><TH>Sensor</TH><TH>Value</TH>");

  for (i = 0 ; i < MAX_SENSORS ; i++) {
    sprintf(&mail.contents[strlen(mail.contents)], 
             "<TR><TH>%1d</TH><TH>%2d</TH>", i, dummySensorData[i]);
  }

  strcat(mail.contents, "</TABLE></CENTER></BODY></HTML>");

  mailSend(&mail);

  return(0);
}


/*
 * Placeholder code for handling commands
 */
int processCommand(char *sender, char *command)
{
  printf("Received command %s from %s\n", command, sender);
  return(0);
}

