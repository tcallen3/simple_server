#include <sys/socket.h>
#include <sys/wait.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "logging.h"
#include "network.h"

void
init_connections(OpenConnections *conn)
{
	conn->sockets = NULL;
	conn->nsock = 0;
}

static int
alloc_socket_list(OpenConnections *conn, nfds_t nsock)
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
	nfds_t i;

	if (conn->sockets != NULL) {
		for (i = 0; i < conn->nsock; i++) {
			(void)close(conn->sockets[i].fd);
		}

		(void)free(conn->sockets);
	}
	conn->sockets = NULL;
	conn->nsock = 0;
}

int
bind_sockets(OpenConnections *conn, const ServerSettings *ss)
{
	struct addrinfo hints, *servinfo, *p;
	nfds_t sock_count;
	int sindex;
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
		fprintf(stderr, "could not allocate space for socket list");
		return -1;
	}

	sindex = -1;
	for (p = servinfo; p != NULL; p = p->ai_next) {
		++sindex;

		if ((conn->sockets[sindex].fd = socket(p->ai_family, 
			p->ai_socktype, p->ai_protocol)) == -1) {
			perror("socket");
			continue;
		}

		if (setsockopt(conn->sockets[sindex].fd, SOL_SOCKET, 
			SO_REUSEADDR, &on_val, sizeof on_val) == -1) {
			perror("setsockopt");
			(void)close(conn->sockets[sindex].fd);
			conn->sockets[sindex].fd = -1;
			continue;
		}

		if (bind(conn->sockets[sindex].fd, p->ai_addr, 
			p->ai_addrlen) == -1) {
			perror("bind");
			(void)close(conn->sockets[sindex].fd);
			conn->sockets[sindex].fd = -1;
			continue;
		}

		if (listen(conn->sockets[sindex].fd, BACKLOG) == -1) {
			perror("listen");
			(void)close(conn->sockets[sindex].fd);
			conn->sockets[sindex].fd = -1;
			continue;
		}

		/* we'll be watching for these to become readable */
		conn->sockets->events = POLLIN;
	}

	freeaddrinfo(servinfo);

	/* make sure at least one socket is valid */
	for (sock_count = 0; sock_count < conn->nsock; sock_count++) {
		if (conn->sockets[sock_count].fd != -1) {
			return 0;
		}
	}

	fprintf(stderr, "%s could not establish connection\n", 
		getprogname());
	return -1;
}

int
poll_connections(OpenConnections *conn)
{
	int rv = 0;
	short errs = POLLERR | POLLHUP | POLLNVAL;
	nfds_t sindex;

	/* not sure if SIGCHLD will interrupt... */
	while ((rv = poll(conn->sockets, conn->nsock, INFTIM)) > 0) {
		for (sindex = 0; sindex < conn->nsock; sindex++) {
			if (conn->sockets[sindex]->revents & errs) {
				/* skip sockets that error */
				/* TODO: refine this based on errno? */
				continue;
			}

			if (conn->sockets[sindex]->revents & POLLIN) {
				/* TODO: add debug codes? */
				accept_connection(conn->sockets[sindex]->fd);
			}
		}

		/* since we woke up in parent, reap children */
		/* NOTE: will set this to sig handler if needed */
		while (waitpid(WAITANY, NULL, WNOHANG) > 0) {
			continue;
		}
	}

	log_debug("unexpected exit from socket polling loop");
	return rv;
}

void 
accept_connection(int sockfd)
{
	int new_sock;
	pid_t pid;
	struct sockaddr_storage client_addr;

	/* TODO: set this to cloexec/reuse? */
	new_sock = accept(sockfd, &client_addr, sizeof client_addr);
	if (new_sock == -1) {
		log_debug("failed to accept request");
		return;
	} 

	if ((pid = fork()) == -1) {
		log_debug("fork error");
		return;
	} else if (pid == 0) {
		/* child */
		/* handle request and exit */
		process_request(new_sock, &client_addr);
		(void)close(new_sock);
		exit(EXIT_SUCCESS);
		/* NOTREACHED */
	} else {
		/* parent */
		/* we'll wait on child in main loop */
		(void)close(new_sock);
		return;
	}
}

void
process_request(int sockfd, const struct sockaddr_storage* client_addr)
{
	char buf[BUFSIZ];
	ssize_t bytes;
	socklen_t size;

	/* NOTE: not sure if this is right usage of client_addr... */
	size = sizeof(*client_addr);
	while ((bytes = recvfrom(sockfd, buf, BUFSIZ-2, 0, 
		(struct sockaddr *)client_addr, &size)) > 0) {
		buf[BUFSIZ-1] = '\0';
		log_debug(buf);
	} 

	if (bytes == -1) {
		log_debug("receiving data from socket failed");
	}
}
