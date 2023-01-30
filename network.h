#ifndef SERVER_NETWORK_H
#define SERVER_NETWORK_H

#include <poll.h>

#include "settings.h"

#define BACKLOG 10

typedef struct OpenConnections {
	struct pollfd *sockets;
	nfds_t nsock;
} OpenConnections;

void init_connections(OpenConnections *);
void destroy_connections(OpenConnections *);

int bind_sockets(OpenConnections *, const ServerSettings *);
int poll_connections(OpenConnections *);
void accept_connection(int);
void process_request(int, const struct sockaddr_storage*);

#endif /* SERVER_NETWORK_H */
