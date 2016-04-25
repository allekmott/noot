/* noot
 * Simple server; binds to port 17007,
 * noots
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#include <unistd.h>

#define NOOT_VERSION "0.0.2"

void error(const char *msg)  {
	fprintf(stderr, "An error occurred: %s\n", msg);
	exit(1);
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
	char *client_ip = inet_ntoa(client_addr->sin_addr); // extract client's ip
	printf("Received connection from %s\n", client_ip);
	printf("Nooting @%s\n", client_ip);

	while (1) {
		if (send_msg(socket_fd, "noot\n") == -1)
			error("could not noot");
		sleep(1);
	}
}

/* Housekeeping, initial network connection */
int init_net() {
	int socket_fd;
	struct sockaddr_in host_addr;
	int yes = 1;
	printf("\nBegin network initialization...\n");
	
	printf("Opening socket\n");
	if ((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1) // stream socket, ip
		error("failed to open socket");

	printf("Making address reusable\n");
	if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
		error("failed to set reusable flag");

	// set up host address info
	host_addr.sin_family = AF_INET; // internet protocol formatted address
	host_addr.sin_port = htons(17007); // input port = host to network string of 17007
	host_addr.sin_addr.s_addr = 0;
	memset(&host_addr.sin_zero, '\0', 8); // zero out zero

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
	int incoming_fd; // socket descriptor for new connection
	struct sockaddr_in client_addr; // client's addressing shiz
	socklen_t sin_size; // size of address struct
	sin_size = sizeof(struct sockaddr_in);

	printf("\nNewtorking initalized, awaiting connections...\n");

	/* loop endlessly, forking process upon new connection */
	while (1) {
		incoming_fd = accept(socket_fd, (struct sockaddr *) &client_addr, &sin_size);
		int pid = fork();
		if (!pid) { // new process
			process_connection(incoming_fd, (struct sockaddr_in *) &client_addr);
		}
	}
}

int main(int argc, char const *argv[]) {
	printf("noot v%s\n", NOOT_VERSION);

	int socket_fd = init_net();
	serve(socket_fd);

	return 0;
}