/**
 * @file	util.h
 * @author	Allek Mott <allekmott@gmail.com>
 * @date	19 September 2018
 * @brief	Utility functions for noot server
 */

#ifndef __NOOT_UTIL_H__
#define __NOOT_UTIL_H__

#include <netinet/in.h>

/* Register handler for SIGCHLD signal */
int register_sigchld(void (*handler) (int));

/*
 * Test if a socket is open
 * 
 * @fd	File descriptor referencing socket
 *
 * Returns 1 if open, 0 if closed
 */
int socket_is_open(int fd);

/* Render string representation of ip address from struct sockaddr_in */
char *s_ip_address(const struct sockaddr_in *address);

/*
 * Write string message to socket
 * 
 * @fd			File descriptor referencing socket
 * @message		String message to be sent
 *
 * Returns 0 on success, negative errno on failure
 */
int send_message(int fd, const char *message);

/*
 * Open server socket
 *
 * @port	Port number to bind to
 *
 * Returns socket file descriptor on success, -errno on failure
 */
int open_server_socket(int port);

#endif /* __NOOT_UTIL_H__ */
