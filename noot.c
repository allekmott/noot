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

#define NOOT_VERSION "0.1.2"

#define DEFAULT_PORT 17007
#define DEFAULT_DELAY_TIME 1000000
#define DEFAULT_CONNECTION_MAX 500

static int connection_max = DEFAULT_CONNECTION_MAX;
static int delay_time = DEFAULT_DELAY_TIME;

static void usage(const char *cmd) {
	printf("Usage: %s [-d delay_time_s]\n", cmd);
	exit(0);
}

/* Handler for SIGCHLD
 * (this is where all connection handlers end)
 */
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

/* Register handler for SIGCHLD */
static int register_sigchld() {
	struct sigaction sa;
	int res;

	memset(&sa, 0x00, sizeof(struct sigaction));
	sa.sa_handler = &handle_sigchld;

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;

	res = sigaction(SIGCHLD, &sa, 0);
	if (res < 0) {
		perror("Could not register SIGCHLD handler");
		return res;
	}

	return 0;
}

/* Checks if a socket is open */
int socket_open(int socket_fd) {
	socklen_t len;
	int error, res;

	error = 0;
	len = sizeof(res);

	/* query socket stuff for error code */
	res = getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &error, &len);

	/* retval & error will be nonzero if an error has occurred,
	 * denoting potential connection issue */
	return (!res && !error);
}

/* Pull IP address from sockaddr_in struct */
static char *s_ip_address(const struct sockaddr_in *client_addr) {
	return inet_ntoa(client_addr->sin_addr);
}

/* Send message over socket */
static int send_message(int fd, const char *message) {
	int to_send, total_sent, sent;

	to_send = strlen(message) + 1;
	total_sent = 0;

	/* while there's still more to send, send it */
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

/* handling of new connection (where the nooting happens) */
void handle_connection(int socket_fd, const struct sockaddr_in *client_addr) {
	char *client_ip;
	int res;

	client_ip = s_ip_address(client_addr);
	printf("Nooting @%s\n", client_ip);

	while (1) {
		if (!socket_open(socket_fd)) {
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

/* Housekeeping, initial network connection */
int init_net() {
	int fd;
	struct sockaddr_in addr;

	int y;

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
	addr.sin_port = htons(DEFAULT_PORT); /* input port (network byte order) */

	printf("Binding to port %i\n", DEFAULT_PORT);

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

	printf("Listening on port %i\n", DEFAULT_PORT);
	return fd;
}

/* Accept and process incoming connections */
void serve(int server_fd) {
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

			printf("%s (%i) disconnected, killing forked process (pid %i)\n",
				s_ip_address(&client_addr), n_connection, getpid());

			close(client_fd);
			kill(getpid(), SIGTERM);
		} else
			printf("-> Connection %i to be handled by process %i\n",
					n_connection, pid);
	}
}

void parse_args(int argc, const char *argv[]) {
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

int main(int argc, char const *argv[]) {
	int server_fd;

	printf("noot v%s\n", NOOT_VERSION);

	/* register SIGCHLD handler (so as to not spread T virus) */
	register_sigchld();

	/* grab delay time, if applicable, from command line args
	 * (stored in global delay_time)
	 */
	parse_args(argc, argv);

	/* print out delay time */
	printf("\nDelay time set to %.1fs\n", delay_time/1000000.0f);

	server_fd = init_net();
	serve(server_fd);

	return 0;
}
