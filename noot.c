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

#define NOOT_VERSION "0.0.1"

int main(int argc, char const *argv[]) {
	printf("noot v%s\n", NOOT_VERSION);
	return 0;
}