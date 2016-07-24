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

#define NOOT_VERSION "0.1.0"

int delay_time = 1000000;
int max_connections = 500;

void usage(const char *cmd) {
	printf("Usage: %s [-d delay_time_s]\n", cmd);
	exit(0);
}

void error(const char *msg)  {
	fprintf(stderr, "An error occurred: %s\n", msg);
	exit(1);
}

/* Handler for SIGCHLD (to aid in termination of zombie processes) */
void handle_sigchld(int sig) {
	int saved_errno = errno;
	while (waitpid((pid_t) (-1), 0, WNOHANG) > 0) {}
	errno = saved_errno;
}

/* Register handler for SIGCHLD */
void register_sigchld() {
	struct sigaction sa;
	sa.sa_handler = &handle_sigchld;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
	if (sigaction(SIGCHLD, &sa, 0) == -1)
		error("Could not register SIGCHLD handler");
}

/* Checks if a socket is open */
int socket_open(int socket_fd) {
	int error = 0;
	socklen_t len = sizeof(error);
	
	/* query socket stuff for error code */
	int retval = getsockopt(socket_fd, 	SOL_SOCKET, SO_ERROR, &error, &len);

	/* retval & error will be nonzero if an error has occurred,
	 * denoting potential connection issue */
	return (!retval && !error); 
}

/* Pull IP address from sockaddr_in struct */
char *socket_ip(struct sockaddr_in *client_addr) {
	return inet_ntoa(client_addr->sin_addr);
}

/* Send message over socket */
int send_msg(int socket_fd, char *buffer) {
	int total_bytes_sent, bytes_sent, total_to_send, left_to_send;

	/* calculate # of bytes to send from length of message */
	total_to_send = left_to_send = strlen(buffer);

	/*printf("Sending %i bytes...\n", total_to_send);*/

	/* while there's still more to send, send it */
	while (left_to_send > 0) {
		bytes_sent = send(socket_fd, buffer, left_to_send, 0);
		total_bytes_sent += bytes_sent;

		/* hopefully the message went through? */
		if (bytes_sent == -1)
			return -1;
		
		left_to_send -= bytes_sent;
		buffer += bytes_sent;
		/*printf("Sent %i/%i bytes\n", total_bytes_sent, total_to_send);*/
	}
	return bytes_sent;
}

/* handling of new connection (where the nooting happens) */
void process_connection(int socket_fd, struct sockaddr_in *client_addr) {
	/* extract client's ip */
	char *client_ip = socket_ip(client_addr);
	printf("Nooting @%s\n", client_ip);

	while (1) {
		/* check to see if socket still active */
		if (!socket_open(socket_fd))
			return;

		/* if so, send noot, sleep for 1s */
		if (send_msg(socket_fd, "noot\r\n") == -1)
			error("could not noot");
		usleep(delay_time);
	}
}

/* Housekeeping, initial network connection */
int init_net() {
	int socket_fd;
	struct sockaddr_in host_addr;
	int yes = 1;
	printf("\nBegin network initialization...\n");
	
	printf("Opening socket\n");
	/* stream socket, ip protocol */
	if ((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
		error("failed to open socket");

	printf("Making address reusable\n");
	if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
		error("failed to set reusable flag");

	/* set up host address info */
	host_addr.sin_family = AF_INET; /* internet protocol formatted address */
	host_addr.sin_port = htons(17007); /* input port = host to network string of 17007 */
	host_addr.sin_addr.s_addr = 0;
	memset(&host_addr.sin_zero, '\0', 8); /* zero out zero */

	printf("Binding to port %i\n", 17007);

	// bind to port
	if (bind(socket_fd, (struct sockaddr *) &host_addr, sizeof(struct sockaddr)) == -1)
		error("failed to bind to port");

	printf("Attempting to listen on port\n");
	if (listen(socket_fd, 5) == -1) // attempt to listen
		error("could not listen on port");

	printf("Listen successful\n");
	return socket_fd;
}

/* Accept and process incoming connections */
void serve(int socket_fd) {
	int con_count = 1;

	int incoming_fd; /* socket descriptor for new connection */
	struct sockaddr_in client_addr; /* client's addressing shiz */
	socklen_t sin_size; /* size of address struct */
	sin_size = sizeof(struct sockaddr_in);

	printf("\nNewtorking initalized, awaiting connections...\n");

	/* loop endlessly, forking process upon new connection */
	while (1) {
		incoming_fd = accept(socket_fd, (struct sockaddr *) &client_addr, &sin_size);
		int con = con_count++;

		/* extract client ip */
		char *client_ip = socket_ip(&client_addr);
		printf("Received connection from %s (#%i)\n", client_ip, con);

		if (con_count > max_connections) {
			con_count--;
			printf("[max connections exceeded, notifying and dropping]\n");
			send_msg(incoming_fd,
				"noot too many connections, wait your turn noot\r\n");
			close(incoming_fd);
			continue;
		}

		int pid = fork();
		if (!pid) { /* new process (pid = 0) */

			process_connection(incoming_fd, (struct sockaddr_in *) &client_addr);
			/* ^ exits upon disconnection */

			printf("%s (%i) disconnected, killing forked process (pid %i)\n",
				socket_ip(&client_addr), con, getpid());

			close(incoming_fd);
			kill(getpid(), SIGTERM);
		} else
			printf("-> Connection %i interfacing with process %i\n", con, pid);
	}
}

void handle_args(int argc, const char *argv[]) {
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
	printf("noot v%s\n", NOOT_VERSION);

	/* register SIGCHLD handler (so as to not spread T virus) */
	register_sigchld();

	/* grab delay time, if applicable, from command line args
	 * (stored in global delay_time)
	 */
	handle_args(argc, argv);

	/* print out delay time */
	printf("\nDelay time set to %.1fs\n", delay_time/1000000.0f);

	int socket_fd = init_net();
	serve(socket_fd);

	return 0;
}
