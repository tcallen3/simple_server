#include <sys/socket.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "network.h"

void
init_connections(OpenConnections *conn)
{
	conn->sockets = NULL;
	conn->nsock = 0;
}

int
alloc_socket_list(OpenConnections *conn, int nsock)
{
	conn->sockets = malloc(nsock * sizeof(*conn->sockets));
	if (conn->sockets == NULL) {
		perror("malloc");
		return -1;
	}
	conn->nsock = nsock;

	return 0;
}

void
destroy_connections(OpenConnections *conn)
{
	if (conn->sockets != NULL) {
		(void)free(conn->sockets);
	}
	conn->nsock = 0;
}

int
bind_sockets(OpenConnections *conn, const ServerSettings *ss)
{
	struct addrinfo hints, *servinfo, *p;
	int sock_count, sindex;
	int rv;
	int on_val = 1;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;	/* IPv4 or IPv6 */
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; 	/* use my IP */
	
	if ((rv = getaddrinfo(ss->ip_address, ss->port, &hints, 
		&servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return -1;
	}

	sock_count = 0;
	for (p = servinfo; p != NULL; p = p->ai_next) {
		sock_count++;
	}

	if (alloc_socket_list(conn, sock_count) == -1) {
		return -1;
	}

	sindex = -1;
	for (p = servinfo; p != NULL; p = p->ai_next) {
		++sindex;

		if ((conn->sockets[sindex] = socket(p->ai_family, 
			p->ai_socktype, p->ai_protocol)) == -1) {
			perror("socket");
			continue;
		}

		if (setsockopt(conn->sockets[sindex], SOL_SOCKET, 
			SO_REUSEADDR, &on_val, sizeof on_val) == -1) {
			perror("setsockopt");
			(void)close(conn->sockets[sindex]);
			conn->sockets[sindex] = -1;
			continue;
		}

		if (bind(conn->sockets[sindex], p->ai_addr, 
			p->ai_addrlen) == -1) {
			perror("bind");
			(void)close(conn->sockets[sindex]);
			conn->sockets[sindex] = -1;
			continue;
		}

		if (listen(conn->sockets[sindex], BACKLOG) == -1) {
			perror("listen");
			(void)close(conn->sockets[sindex]);
			conn->sockets[sindex] = -1;
			continue;
		}
	}

	freeaddrinfo(servinfo);

	/* make sure at least one socket is valid */
	for (sock_count = 0; sock_count < conn->nsock; sock_count++) {
		if (conn->sockets[sock_count] != -1) {
			return 0;
		}
	}

	fprintf(stderr, "%s could not establish connection", 
		getprogname());
	return -1;
}
