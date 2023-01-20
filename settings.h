#ifndef SERVER_SETTINGS_H
#define SERVER_SETTINGS_H

#include <stddef.h>

#define PORT "8080"

typedef struct SeverSettings {
	char *cgi_dir;
	char *ip_address;
	char *log_file;
	char *port;

	int debug;
} ServerSettings;

void set_defaults(ServerSettings *);

#endif /* SERVER_SETTINGS_H */
