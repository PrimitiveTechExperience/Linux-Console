#ifndef __COMMANDS_H__
#define __COMMANDS_H__

 // Commands
#define MAX_BG_PROCESSES 4096

void execute_commands(char **tokens, int token_count);
void handle_sigchld(int sig);
void execute_command(char *command, char **args);
int execute_bin_command(char *command, char **args);
ssize_t print_background_processes();
//Sockets
#ifndef SERVER_PORT
    #define SERVER_PORT 30000
#endif

#ifndef MAX_CONNECTIONS
    #define MAX_CONNECTIONS 100000
#endif

#ifndef MAX_BACKLOG
    #define MAX_BACKLOG 5
#endif

#ifndef MAX_NAME
    #define MAX_NAME 10
#endif

#ifndef MAX_USER_MSG
    #define MAX_USER_MSG 128
#endif

/*
 * Under our chat protocol, the maximum size of a message sent by
 * a server is MAX(username) + space + MAX(user message) + CRLF
 */
#ifndef MAX_PROTO_MSG
    #define MAX_PROTO_MSG MAX_NAME+1+MAX_USER_MSG+2
#endif

/* Working with string functions to parse/manipulate the contents of
 * the buffer can be convenient. Since we are using a text-based
 * protocol (i.e., message contents will consist only of valid ASCII
 * characters) let's leave 1 extra byte to add a NULL terminator
 * so that we can more easily use string functions. We will never
 * actually send a NULL terminator over the socket though.
 */
#ifndef BUF_SIZE
    #define BUF_SIZE MAX_PROTO_MSG+1 
#endif

struct listen_sock {
    struct sockaddr_in *addr;
    int sock_fd;
};

/*
 * Initialize a server address associated with the required port.
 * Create and setup a socket for a server to listen on.
 */
void setup_server_socket(struct listen_sock *s, int port);

/*
 * Search the first n characters of buf for a network newline (\r\n).
 * Return one plus the index of the '\n' of the first network newline,
 * or -1 if no network newline is found.
 * Definitely do not use strchr or other string functions to search here. (Why not?)
 */
int find_network_newline(const char *buf, int n);

/* 
 * Reads from socket sock_fd into buffer *buf containing *inbuf bytes
 * of data. Updates *inbuf after reading from socket.
 *
 * Return -1 if read error or maximum message size is exceeded.
 * Return 0 upon receipt of CRLF-terminated message.
 * Return 1 if socket has been closed.
 * Return 2 upon receipt of partial (non-CRLF-terminated) message.
 */
int read_from_socket(int sock_fd, char *buf, int *inbuf);

/*
 * Search src for a network newline, and copy complete message
 * into a newly-allocated NULL-terminated string **dst.
 * Remove the complete message from the *src buffer by moving
 * the remaining content of the buffer to the front.
 *
 * Return 0 on success, 1 on error.
 */
int get_message(char **dst, char *src, int *inbuf);

/*
 * Write a string to a socket.
 *
 * Return 0 on success.
 * Return 1 on error.
 * Return 2 on disconnect.
 * 
 * See Robert Love Linux System Programming 2e p. 37 for relevant details
 */
int write_to_socket(int sock_fd, char *buf, int len);
 // chat helpers.
#include <netdb.h>

struct client_sock {
    int sock_fd;
    int state;
    int id;
    char buf[BUF_SIZE];
    char ip_address[INET_ADDRSTRLEN];
    int inbuf;
    struct client_sock *next;
};

/* 
 * Send a string to a client.
 * 
 * Input buffer must contain a NULL-terminated string. The NULL
 * terminator is replaced with a network-newline (CRLF) before
 * being sent to the client.
 * 
 * On success, return 0.
 * On error, return 1.
 * On client disconnect, return 2.
 */
int write_buf_to_client(struct client_sock *c, char *buf, int len);

/* 
 * Remove client from list. Return 0 on success, 1 on failure.
 * Update curr pointer to the new node at the index of the removed node.
 * Update clients pointer if head node was removed.
 */
int remove_client(struct client_sock **curr, struct client_sock **clients);

/* 
 * Read incoming bytes from client.
 * 
 * Return -1 if read error or maximum message size is exceeded.
 * Return 0 upon receipt of CRLF-terminated message.
 * Return 1 if client socket has been closed.
 * Return 2 upon receipt of partial (non-CRLF-terminated) message.
 */
int read_from_client(struct client_sock *curr);

/* Set a client's user name.
 * Returns 0 on success.
 * Returns 1 on either get_message() failure or
 * if user name contains invalid character(s).
 */
int set_username(struct client_sock *curr);

void start_server(int port);
void start_client(int port, const char *hostname);

#endif
