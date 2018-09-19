/**
 * @file	util.c
 * @author	Allek Mott <allekmott@gmail.com>
 * @date	19 September 2018
 * @brief	Implementation of noot server utility functions
 */

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "util.h"

int register_sigchld(void (*handler) (int)) {
	struct sigaction sa;
	int res;

	memset(&sa, 0x00, sizeof(struct sigaction));
	sa.sa_handler = handler;

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;

	res = sigaction(SIGCHLD, &sa, 0);
	if (res < 0) {
		perror("Could not register SIGCHLD handler");
		return res;
	}

	return 0;
}

int socket_is_open(int fd) {
	socklen_t len;
	int error, res;

	error = 0;
	len = sizeof(res);

	/* query socket stuff for error code */
	res = getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len);

	/* retval & error will be nonzero if an error has occurred,
	 * denoting potential connection issue */
	return (!res && !error);
}

char *s_ip_address(const struct sockaddr_in *address) {
	return inet_ntoa(address->sin_addr);
}

int send_message(int fd, const char *message) {
	int to_send, total_sent, sent;

	to_send = strlen(message) + 1;
	total_sent = 0;

	while (total_sent < to_send) {
		sent = send(fd, message, to_send - total_sent, 0);

		/* hopefully the message went through? */
		if (sent == -1) {
			return -(errno);
		}

		total_sent += sent;
		message += sent;
	}

	return total_sent;
}

int open_server_socket(int port) {
	int fd;
	struct sockaddr_in addr;

	int y;

	/* TODO: move print statements elsewhere */

	y = 1;
	printf("\nBegin network initialization...\n");

	printf("Opening socket\n");
	/* stream socket, ip protocol */
	if ((fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("failed to open socket");
		return -(errno);
	}

	printf("Configuring address\n");
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(int)) < 0) {
		perror("failed to set reusable address configurations");
		return -(errno);
	}

	memset(&addr, 0x00, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET; /* internet protocol formatted address */
	addr.sin_port = htons(port); /* input port (network byte order) */

	printf("Binding to port %i\n", port);

	/* bind to port */
	if (bind(fd, (struct sockaddr *) &addr, sizeof(struct sockaddr)) == -1) {
		perror("failed to bind to port");
		return -(errno);
	}

	printf("Attempting to listen on port\n");
	if (listen(fd, 5) == -1) {
		perror("could not listen on port");
		return -(errno);
	}

	printf("Listening on port %i\n", port);
	return fd;
}
