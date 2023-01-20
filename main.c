#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "settings.h"

void
usage(FILE *fp)
{
	fprintf(fp, "%s [-dh] [-c dir] [-i addr] [-l file] [-p port] dir\n",
		getprogname());

	fprintf(fp, "%s serves content from the specified directory, dir\n",
		getprogname());
	fprintf(fp, "In addition, it supports the following options:\n");

	fprintf(fp, "\t-c cgidir \tAllow execution of CGIs from cgidir\n");
	fprintf(fp, "\t-d        \tEnter debugging mode\n");
	fprintf(fp, "\t-h        \tPrint this summary and exit\n");
	fprintf(fp, "\t-i addr   \tBind to (IPv4 or IPv6) address\n");
	fprintf(fp, "\t-l file   \tLog all requests to given file\n");
	fprintf(fp, "\t-p port   \tListen on the given port\n");
}

int
main(int argc, char *argv[])
{
	int ch;
	const char *all_opts = "c:dhi:l:p:";
	ServerSettings settings;

	setprogname(argv[0]);

	if (argc < 2) {
		usage(stderr);
		exit(EXIT_FAILURE);
	}

	set_defaults(&settings);

	while ((ch = getopt(argc, argv, all_opts)) != -1) {
		switch (ch) {
		case 'c':
			settings.cgi_dir = optarg;
			break;
		case 'd':
			settings.debug = 1;
			break;
		case 'h':
			usage(stdout);
			exit(EXIT_SUCCESS);
			/* NOTREACHED */
			break;
		case 'i':
			settings.ip_address = optarg;
			break;
		case 'l':
			settings.log_file = optarg;
			break;
		case 'p':
			settings.port = optarg;
			break;
		case '?':
			/* FALLTHROUGH */
		default:
			usage(stderr);
			exit(EXIT_FAILURE);
			/* NOTREACHED */
			break;
		}
	}

	/* !! BEGIN TEST !! */

	/* !! END TEST !! */

	return EXIT_SUCCESS;
}
