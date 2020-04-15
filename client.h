/*
 * client.h
 */

#define MAX_SUBJECT		80
#define MAX_RCPT		80
#define MAX_CONTENT_TYPE	80
#define MAX_SPECIAL		256
#define MAX_CONTENT		2048

struct mailHeader {
	char subject[MAX_SUBJECT+1];
	char mailRcpt[MAX_RCPT+1];
	char specialHeaders[MAX_SPECIAL+1];
	char contentType[MAX_CONTENT_TYPE+1];
	char contents[MAX_CONTENT+1];
};

int mailSend(struct mailHeader *);

/*
 * System Configurable Parameters Below:
 */
#define SMTPGATEWAY		"192.168.1.1"
#define SOURCE_EMAIL_ADDRESS	"device@mydomain.home"

