#include "logging.h"

static int debug = 0;
static FILE* logfile = NULL;

void
setup_logging(const ServerSettings* settings)
{
	debug = settings->debug;

	if (debug) {
		/* set this here so user-specified file overwrites */
		logfile = stderr;
	}

	if (settings->log_file != NULL) {
		logfile = fopen(settings->log_file, "a");
		if (logfile == NULL) {
			perror("fopen");
			fprintf(stderr, "server will not log requests\n");
		}
	}
}

void
log_debug(const char* msg)
{
	if (!debug) {
		return;
	}

	fprintf(logfile, msg);
}
