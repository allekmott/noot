/* noot
 * Simple server; binds to port 17007,
 * noots
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "util.h"

#define NOOT_VERSION "0.1.3"

#define DEFAULT_PORT 17007
#define DEFAULT_DELAY_TIME 1000000
#define DEFAULT_CONNECTION_MAX 500

static int connection_max = DEFAULT_CONNECTION_MAX;
static int delay_time = DEFAULT_DELAY_TIME;

static void usage(const char *cmd) {
	printf("Usage: %s [-d delay_time_s]\n", cmd);
	exit(0);
}

static void handle_sigchld(int sig);
static void handle_connection(
		int socket_fd, const struct sockaddr_in *client_addr);
static void serve(int socket_fd);
static void parse_args(int argc, const char *argv[]);

int main(int argc, char const *argv[]) {
	int server_fd;

	printf("noot v%s\n", NOOT_VERSION);

	/* register SIGCHLD handler (so as to not spread T virus) */
	register_sigchld(handle_sigchld);

	/* grab delay time, if applicable, from command line args
	 * (stored in global delay_time)
	 */
	parse_args(argc, argv);

	/* print out delay time */
	printf("\nDelay time set to %.1fs\n", delay_time/1000000.0f);

	server_fd = open_server_socket(DEFAULT_PORT);
	serve(server_fd);

	return 0;
}

/* Destroy child process on exit (to evade zombie process buildup) s*/
static void handle_sigchld(int sig) {
	/* preserve error state */
	int status;
	pid_t pid;

	int initial_errno;

	switch (sig) {
		case SIGCHLD:
			initial_errno = errno;
			
			/* wait for process to terminate */
			while ((pid = waitpid((pid_t) (-1), &status, WNOHANG)) > 0) ;
			
			errno = initial_errno;
		default: return;
	}
}

/* Handle client connection */
static void handle_connection(
		int socket_fd, const struct sockaddr_in *client_addr) {

	char *client_ip;
	int res;

	client_ip = s_ip_address(client_addr);
	printf("Nooting @%s\n", client_ip);

	while (1) {
		if (!socket_is_open(socket_fd)) {
			return;
		}
		
		res = send_message(socket_fd, "noot\r\n");
		if (res < 0) {
			perror("could not noot");
			return;
		}

		usleep(delay_time);
	}
}

/* Main listening loop; accept and handle client connections */
static void serve(int server_fd) {
	int client_fd;
	char *client_ip;
	struct sockaddr_in client_addr;
	socklen_t client_addr_size;

	int n_connections, n_connection;

	n_connections = 0;
	client_addr_size = sizeof(struct sockaddr_in);

	printf("\nNewtorking initalized, awaiting connections...\n");

	/* loop endlessly, forking process upon new connection */
	while (1) {
		client_fd = accept(server_fd,
				(struct sockaddr *) &client_addr, &client_addr_size);

		if (n_connections == connection_max) {
			printf("[max connections exceeded, notifying and dropping]\n");
			send_message(client_fd,
					"noot too many connections, wait your turn noot\r\n");
			close(client_fd);
			continue;
		}

		n_connection = ++n_connections;

		client_ip = s_ip_address(&client_addr);
		printf("Received connection from %s (#%i)\n", client_ip, n_connection);

		int pid = fork();
		if (!pid) { /* new process (pid = 0) */

			handle_connection(client_fd, (struct sockaddr_in *) &client_addr);
			/* ^ exits upon disconnection */

			printf("%s (%i) disconnected, killing child process (pid %i)\n",
					s_ip_address(&client_addr), n_connection, getpid());

			close(client_fd);
			kill(getpid(), SIGTERM);
		} else
			printf("-> Connection %i to be handled by process %i\n",
					n_connection, pid);
	}
}

/*
 * Parse command line arguments using getopt
 *
 * TODO: do this in main & replace global variables with something less sketchy
 */
static void parse_args(int argc, const char *argv[]) {
	int c;
	extern int optind, optopt;

	/* iterate through command line flags */
	while ((c = getopt(argc, (char *const *) argv, "d:h")) != -1) {
		switch(c) {
			/* d flag -> set delay time in seconds */
			case 'd':
				delay_time = atof(optarg) * 1000000;
				if (delay_time <= 0) {
					printf("Invalid delay time: %.1fs\n", delay_time/1000000.0f);
					exit(1);
				}
				break;

			/* help/invalid flag -> usage */
			case 'h':
			case '?': usage(argv[0]); break;
		}
	}
}

