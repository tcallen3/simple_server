#ifndef SERVER_NETWORK_H
#define SERVER_NETWORK_H

#include "settings.h"

#define BACKLOG 10

typedef struct OpenConnections {
	int *sockets;
	int nsock;
} OpenConnections;

int init_connections(OpenConnections *);
void destroy_connections(OpenConnections *);

int alloc_socket_list(OpenConnections *, int);

int bind_sockets(const ServerSettings *);

#endif /* SERVER_NETWORK_H */
