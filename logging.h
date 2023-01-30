#ifndef SERVER_LOGGING_H
#define SERVER_LOGGING_H

#include <stdio.h>

void setup_logging(const ServerSettings*); 
void log_debug(const char*);

#endif /* SERVER_LOGGING_H */
