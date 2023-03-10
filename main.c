/*

BSD 3-Clause License

Copyright (c) 2023, Thomas Allen

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "logging.h"
#include "network.h"
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
	int rv = 0;
	const char *all_opts = "c:dhi:l:p:";
	ServerSettings settings;
	OpenConnections connections;

	setprogname(argv[0]);

	set_defaults(&settings);
	init_connections(&connections);

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

	argc -= optind;
	argv += optind;

	if (argc != 1) {
		usage(stderr);
		exit(EXIT_FAILURE);
	}

	setup_logging(&settings);

	settings.server_dir = argv[0];
	if (chdir(settings.server_dir) == -1) {
		perror("chdir");
		exit(EXIT_FAILURE);
	}

	if (bind_sockets(&connections, &settings) == -1) {
		exit(EXIT_FAILURE);
	}

	if (!settings.debug) {
		/* we already changed to working dir */
		if (daemon(1, 0) == -1) {
			perror("daemonization");
			exit(EXIT_FAILURE);
		}
	}

	rv = poll_connections(&connections);

	destroy_connections(&connections);

	return rv;
}
