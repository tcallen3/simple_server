#include "settings.h"

void 
set_defaults(ServerSettings *s)
{
	s->cgi_dir = NULL;
	s->ip_address = NULL;
	s->log_file = NULL;
	s->port = PORT;

	s->debug = 0;
}
