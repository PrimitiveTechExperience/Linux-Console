#include "builtins.h"
#include "commands.h"
#include "variables.h"
#include "io_helpers.h"

typedef struct {
    pid_t pid;
    int job_number;
    char command[256];
} bg_process;

bg_process bg_processes[MAX_BG_PROCESSES];
int bg_process_count = 0;

void handle_sigchld(int sig) {
    int status;
    pid_t pid;
    // if the signal is anything but a sigchld, return none
    if (sig != SIGCHLD) {
        return;
    }
    (void) sig;
    // Reap all terminated child processes
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < bg_process_count; i++) {
            if (bg_processes[i].pid == pid) {
                char message[512];
                snprintf(message, sizeof(message), "[%d]+  Done %s\n", 
                         bg_processes[i].job_number, 
                         bg_processes[i].command);
                display_message(message);
                // display_message("\n");

                // Remove the process from the list
                for (int j = i; j < bg_process_count - 1; j++) {
                    bg_processes[j] = bg_processes[j + 1];
                }
                bg_process_count--;
                print_path();
                break;
            }
        }
    }
}

ssize_t print_background_processes() {
    for (int i = 0; i < bg_process_count; i++) {
        char message[MAX_BG_PROCESSES];
        snprintf(message, sizeof(message), "%s %d\n", 
                 bg_processes[i].command,
                 bg_processes[i].pid);
        display_message(message);
    }
    return 0;
}

void execute_commands(char **tokens, int token_count){
    // resgister the signal handler for SIGCHLD
    signal(SIGCHLD, handle_sigchld);
    // need to check if there are any pipes:
    // if there are no pipes, then we can execute the commands as normal.
    // track the background status as well.
    int pipeCount = 0;
    int bg = 0;
    // check for the ampersand at the end of the command:
    if (strchr(tokens[token_count - 1], '&') != NULL && strlen(tokens[token_count - 1]) == 1){
        bg = 1;
        // free the ampersand from the command:
        free(tokens[token_count - 1]);
        // set the last token to NULL.
        tokens[token_count - 1] = NULL;
        token_count--;
    }
    // check for pipes:
    for(int i = 0; i < token_count; i++){
        if (strchr(tokens[i], '|') != NULL){
            // count the number of pipes using a for loop going over each char:
            for(size_t j = 0; j < strlen(tokens[i]); j++){
                if(tokens[i][j] == '|'){
                    pipeCount++;
                }
            }
        }
    }
    // display_message("pipe count: ");
    // char pipeCounter[128];
    // sprintf(pipeCounter, "%d", pipeCount);
    // display_message(pipeCounter);
    // display_message("\n");
    if(pipeCount == 0){
        // if background has been requested, then we need to fork the process.
        if(bg == 1){
            pid_t pid = fork();
            // fork error.
            if(pid == -1){
                display_error("ERROR: Fork failed", "");
                return;
            }else if(pid == 0){
                // child process
                // ignore sigint
                signal(SIGINT, SIG_IGN);
                // execute the command:
                execute_command(tokens[0], tokens);
                // if execute_command fails, then we exit the child process.
                exit(1);
            }else{
                // parent process
                // add the process to the list of background processes.
                char message[512];
                snprintf(message, sizeof(message), "[%d] %d\n", bg_process_count + 1, pid);
                display_message(message);
                bg_processes[bg_process_count].pid = pid;
                bg_processes[bg_process_count].job_number = bg_process_count + 1;
                char command[512];
                int ind = 0;
                command[0] = '\0';
                if (tokens[ind] != NULL){
                    strncat(command, tokens[ind], strlen(tokens[ind]));
                    ind++;
                }
                while(tokens[ind] != NULL){
                    strncat(command, " ", 2);
                    strncat(command, tokens[ind], strlen(tokens[ind]));
                    ind++;
                }
                strcpy(bg_processes[bg_process_count].command, command);
                bg_process_count++;
                // display_message("Background process added\n");
            }
        }else{
            // proceed as usual.
            execute_command(tokens[0], tokens);
        }
    }else{
        // display_message("Pipes detected\n");
        // there are pipes, so we need to route the outputs of the previous command into the input of the next command.
        // grab the commands first by creating a sanitized version of the tokens array, then splitting on the pipes using strtok.
        char *sanitized[token_count];
        for(int i = 0; i < token_count; i++){
            sanitized[i] = strdup(tokens[i]);
        }
        // losing my mind, 2 index.
        char *commands[pipeCount + 2][token_count + 2];
        //memset the commands:
        for(int i = 0; i < pipeCount + 2; i++){
            for(int j = 0; j < token_count+2; j++){
                commands[i][j] = NULL;
            }
        }
        int commandCount = 0;
        int argCount = 0;
        int cmdl = 0;
        //check if the first char of the first token is a pipe, if so, throw an error.
        if (tokens[0][0] == '|'){
            display_error("ERROR: Invalid command", "");
            return;
        }
        for(int i = 0; i < token_count; i++){
            if (strchr(tokens[i], '|') != NULL){
                // check if it is a singular pipe:
                if(strlen(tokens[i]) == 1){
                    // just update the command count, arg count, and move on
                    commandCount++;
                    argCount = 0;
                    cmdl = 0;
                    continue;
                }
                // we are going to do this the "python way".
                // pipes are basically variable expansions, so we can treat them as such.
                // first, create temp str array of size tokens[i] + 1 (for null term.)
                char tempStr[strlen(tokens[i]) + 1];
                // set the first char to null.
                tempStr[0] = '\0';
                // using a for loop, iterate over all the chars in tokens[i]
                for(size_t j = 0; j < strlen(tokens[i]); j++){
                    // if we find a pipe, then we know that we have reached the end of the command.
                    if(tokens[i][j] == '|'){
                        // copy the tempStr into the commands array.
                        if(tempStr[0] != '\0'){
                            commands[commandCount][argCount] = strndup(tempStr, cmdl);
                        }
                        // increment the command count.
                        commandCount++;
                        // reset the tempStr.
                        tempStr[0] = '\0';
                        // reset the argCount.
                        argCount = 0;
                        // reset the cmdl.
                        cmdl = 0;
                    }else{
                        // if we have not reached the end of the command, then we can continue copying the command.
                        strncat(tempStr, &tokens[i][j], 1);
                        cmdl++;
                    }
                }
                // // if the last command is just the null terminator, throw an error.
                // if (tempStr[0] == '\0'){
                //     display_error("ERROR: Invalid command", "");
                //     return;
                // }
                // copy the last command into the array, given that the array has non-zero length:
                if (cmdl){
                    commands[commandCount][argCount] = strndup(tempStr, cmdl);
                }
            }else{
                // we proceed as usual:
                commands[commandCount][argCount] = strndup(sanitized[i], strlen(sanitized[i]));
                argCount++;
            }
            // printf("Command count: %d\nArgument count: %d\n", commandCount, argCount);
        }
        // DEBUG CODE: CHECK THE COMMANDS ARRAY TO MAKE SURE THAT COMMANDS ARE PROPERLY PARSED.

        // char message[128];
        // snprintf(message, sizeof(message), "Current command count: %d\n", commandCount);
        // display_message(message);
        // snprintf(message, sizeof(message), "Current arg count: %d\n", argCount);
        // display_message(message);
        // // display_message("Full first command: ");
        // for (int j = 0; j <= commandCount ; j++){
        //     display_message("Command number ");
        //     char commandNumber[128];
        //     sprintf(commandNumber, "%d", j);
        //     display_message(commandNumber);
        //     display_message(": ");
        //     for (int i = 0; i <= token_count; i++){
        //         if(commands[j][i] == NULL){
        //             break;
        //         }
        //         display_message(commands[j][i]);
        //         display_message(" ");
        //     }
        //     display_message("\n");
        // }

        // END DEBUG CODE

        // now that we have all commands stored in the commands array, we can start the piping process.
        // create a pipe for each pipe:
        int pipefds[pipeCount + 1][2];
        for(int i = 0; i < pipeCount + 1; i++){
            if(pipe(pipefds[i]) == -1){
                display_error("ERROR: Pipe failed", "");
                return;
            }
        }
        // create a child process for each command, and connect the pipes.
        for(int i = 0; i < pipeCount+1; i++){
            pid_t pid = fork();
            if (pid == -1) {
                display_error("ERROR: Fork failed", "");
                return;
            } else if (pid == 0) {
                // child
                //ignore sigint
                signal(SIGINT, SIG_IGN);
                if (i > 0) {
                    // not first, redir input
                    dup2(pipefds[i - 1][0], STDIN_FILENO);
                }
                if (i < pipeCount) {
                    // not last, redir output
                    dup2(pipefds[i][1], STDOUT_FILENO);
                }
                // close all fds.
                for (int j = 0; j < pipeCount + 1; j++) {
                    close(pipefds[j][0]);
                    close(pipefds[j][1]);
                }
                // execute all commands.
                if (commands[i][0] != NULL){
                    execute_command(commands[i][0], commands[i]);
                }
                exit(1);
            } else if (bg && i == commandCount - 1) {
                // parent process plus background.
                char message[512];
                // display the message of command number and pid.
                snprintf(message, sizeof(message), "[%d] %d\n", bg_process_count + 1, pid);
                display_message(message);
                // add to the linked list the pid, num and cmd.
                bg_processes[bg_process_count].pid = pid;
                bg_processes[bg_process_count].job_number = bg_process_count + 1;
                char command[512];
                int ind = 0;
                command[ind] = '\0';
                if (tokens[ind]!=NULL){
                    strncat(command, tokens[ind], strlen(tokens[ind]));
                    ind++;
                }
                while(tokens[ind] != NULL){
                    strncat(command, " ", 2);
                    strncat(command, tokens[ind], strlen(tokens[ind]));
                    ind++;
                }
                strcpy(bg_processes[bg_process_count].command, command);
                bg_process_count++;
            }
        }
        // close all the pipes in the parent process:
        for(int i = 0; i < pipeCount + 1; i++){
            close(pipefds[i][0]);
            close(pipefds[i][1]);
        }
        // wait for all the child processes to finish:
        if (!bg) {
            for (int i = 0; i < commandCount; i++) {
                wait(NULL);
            }
        }
        // display_message("Commands finished\n");
        // free all of sanitized.
        for(int  i= 0 ; i < token_count; i++){
		    free(sanitized[i]);
	    }
       	// free all of commands.
        for(int i = 0; i < pipeCount + 2; i++){
            for(int j = 0; j < token_count+2; j++){
                if(commands[i][j] != NULL){
                    free(commands[i][j]);
                }
            }
        }
    }
}


void execute_command(char *command, char **args){
    bn_ptr builtin_fn = check_builtin(command);
    if (builtin_fn != NULL){        
        ssize_t err = builtin_fn(args);
        if (err == - 1) {
            display_error("ERROR: Builtin failed: ", command);
        }
        // check for variable assignment.
    }else if (strchr(command, '=') != NULL && !(strchr(command, '$') != NULL && (strchr(command, '=') > strchr(command, '$')))){
        char *name = strtok(command, "=");
        char *value = strtok(NULL, "");
        if(value == NULL){
            value = "";
        }
        updateVar(name, value);
    }else {
        // call the bin command execution function
        int status = execute_bin_command(command, args);
        if (status == 1) {
            display_error("ERROR: Unknown command: ", command);
        } else if (status == -2) {
            display_error("ERROR: Command failed: ", command);
        }
    }
}

int execute_bin_command(char *command, char **args) {
    char path[512];
    // try executable.
    if (strncmp(command, "./", 2) == 0) {
        if (access(command, X_OK) == 0) {
            pid_t pid = fork();
            if (pid < 0){
                return -2;
            }
            if (pid == 0) {
                execv(command, args);
                exit(1); 
            } else {
                int status;
                waitpid(pid, &status, 0);
                return WEXITSTATUS(status);
            }
        } else {
            display_error("ERROR: Command not found or not executable: ", command);
            return -1;
        }
    }
    // Try /bin/
    snprintf(path, sizeof(path), "/bin/%s", command);
    if (access(path, X_OK) == 0) {
        pid_t pid = fork();
        if (pid < 0){
            return -2;
        }
        if (pid == 0) {
            execv(path, args);
            exit(1);  // If execv fails
        } else {
            int status;
            waitpid(pid, &status, 0);
            return WEXITSTATUS(status);
        }
    }
    // If not in /bin/, try /usr/bin/
    snprintf(path, sizeof(path), "/usr/bin/%s", command);
    if (access(path, X_OK) == 0) {
        pid_t pid = fork();
        if (pid < 0){
            return -2;
        }
        if (pid == 0) {
            execv(path, args);
            exit(1);
        } else {
            int status;
            waitpid(pid, &status, 0);
            return WEXITSTATUS(status);
        }
    }
    // If not found anywhere
    return 1; // indicate unknown command
}

// Sockets:
// Code taken directly from w10's lab.
void setup_server_socket(struct listen_sock *s, int port) {
    if (!(s->addr = malloc(sizeof(struct sockaddr_in)))) {
        perror("malloc");
        exit(1);
    }

    // Allow sockets across machines.
    s->addr->sin_family = AF_INET;
    // The port the process will listen on.
    s->addr->sin_port = htons(port);
    // Clear this field; sin_zero is used for padding for the struct.
    memset(&(s->addr->sin_zero), 0, 8);
    // Listen on all network interfaces.
    s->addr->sin_addr.s_addr = INADDR_ANY;

    s->sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (s->sock_fd < 0) {
        perror("server socket");
        exit(1);
    }

    // Enable SO_REUSEADDR to allow immediate reuse of the port
    int optval = 1;
    if (setsockopt(s->sock_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        perror("setsockopt SO_REUSEADDR");
        exit(1);
    }

    if (setsockopt(s->sock_fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) < 0) {
        perror("setsockopt SO_REUSEPORT");
        exit(1);
    }

    // Bind the selected port to the socket.
    // printf("DEBUG: attempting to bind to socket...\n\n");
    if (bind(s->sock_fd, (struct sockaddr *)s->addr, sizeof(*(s->addr))) < 0) {
        perror("server: bind");
        close(s->sock_fd);
        exit(1);
    }
    // Announce willingness to accept connections on this socket.
    if (listen(s->sock_fd, MAX_BACKLOG) < 0) {
        perror("server: listen");
        close(s->sock_fd);
        exit(1);
    }
}

/* Insert helper functions from last week here. */

int find_network_newline(const char *buf, int inbuf) {
    // do simple for loop.
    for (int i = 0; i < inbuf - 1; i++) {
        if (buf[i] == '\r' && buf[i + 1] == '\n') {
            return i + 2;
        }
    }
    return -1;
}

int read_from_socket(int sock_fd, char *buf, int *inbuf) {
    int nbytes = read(sock_fd, buf + *inbuf, BUF_SIZE - *inbuf);
    if (nbytes < 0) {
        perror("read");
        return -1;
    } else if (nbytes == 0) {
        // check to make sure that the client actually disconnected.
        if (*inbuf == 0) {
            // client disconnected.
            return 1; 
        } else {
            // client not disconnected, but too much to read.
            return 2; 
        }
    }

    *inbuf += nbytes;
    // printf("DEBUG: Read %d bytes from socket %d. Total in buffer: %d\n", nbytes, sock_fd, *inbuf);
    if (*inbuf >= BUF_SIZE) {
        display_error("ERROR: Buffer overflow", "");
        return -1;
    }

    if (find_network_newline(buf, *inbuf) == -1) {
        return 2;
    }
    return 0;
}

int get_message(char **dst, char *src, int *inbuf) {
    // printf("DEBUG: Inbuf at %d\n", *inbuf);
    int newline = find_network_newline(src, *inbuf); 
    if (newline == -1) {
        return 1;
    }
    // remove the \r\n
    int message_length = newline - 2;
    // truncate the message if it is too long.
    if (message_length > MAX_USER_MSG) {
        message_length = MAX_USER_MSG;
    }
    // make new msg.
    *dst = malloc(message_length + 1);
    if (*dst == NULL) {
        perror("malloc");
        return 1;
    }
    // copy only msg, not newline stuff
    memcpy(*dst, src, message_length);
    (*dst)[message_length] = '\0';
    // remove msg, newline stuff from buf
    memmove(src, src + newline, *inbuf - newline);
    *inbuf -= newline;
    return 0;
}

/* Helper function to be completed for this week. */

int write_to_socket(int sock_fd, char *buf, int len) {
    int bytes_written = 0;
    while (bytes_written < len) {
        int n = write(sock_fd, buf + bytes_written, len - bytes_written);
        if (n < 0) {
            // ensure that interrupts are handled correctly.
            if (errno == EINTR) {
                continue;
            }
            perror("write");
            return 1;
        }
        if (n == 0) {
            // disconnection.
            return 2;
        }
        bytes_written += n;
        // printf("DEBUG: Wrote %d bytes to socket %d. Total written: %d\n", n, sock_fd, bytes_written);
    }
    return 0;
}

// chat helpers.
// code directly taken from w10's lab.
int write_buf_to_client(struct client_sock *c, char *buf, int len) {
    // prelim check for invalid input.
    if (c == NULL || buf == NULL || len <= 0) {
        display_error("Error: Invalid input to write_buf_to_client", "");
        return 1;
    }
    // too long of a message.
    if (len + 2 > BUF_SIZE) {
        display_error("Error: Message too long to send to client", "");
        return 1;
    }
    // copy the message to a temp buffer, and add the \r\n.
    char temp_buf[BUF_SIZE];
    memcpy(temp_buf, buf, len);
    temp_buf[len] = '\r';
    temp_buf[len + 1] = '\n';

    // printf("DEBUG: Sending message to client %d: %.*s\n", c->sock_fd, len + 2, temp_buf);
    return write_to_socket(c->sock_fd, temp_buf, len + 2);
}

int remove_client(struct client_sock **curr, struct client_sock **clients) {
    // no clients, no need to remove.
    if (*clients == NULL) {
        return 1;
    }
    // treat the clients list as a linked list, need to replace the client with the next client.
    struct client_sock *temp = *clients;
    struct client_sock *prev = NULL;
    // classic linked list traversal.
    while (temp != NULL) {
        if (temp == *curr) {
            // client found.
            if (prev == NULL) {
                // indicates that this is the first client, so we need to update the head of the list.
                *clients = temp->next;
            } else {
                // removing a client in the middle or end of the list
                prev->next = temp->next;
            }

            // remember to close socket, free mem.
            close(temp->sock_fd);
            free(temp);
            // safety.
            *curr = NULL; 
            return 0;
        }
        prev = temp;
        temp = temp->next;
    }
    // only reached if client was never found.
    return 1; 
}

int read_from_client(struct client_sock *curr) {
    return read_from_socket(curr->sock_fd, curr->buf, &(curr->inbuf));
}

int set_username(struct client_sock *curr) {
    // username not needed. suppress unused variable warning.
    (void) curr;
    return 0;
}
// Clients
// code directly taken from w10's lab.
void start_client(int port, const char *hostname) {
    struct sockaddr_in server;
    int sock_fd;
    char buf[BUF_SIZE];
    int inbuf = 0;
    char strbuf[1000];

    // create socket fd
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("client: socket");
        exit(1);
    }

    // server address
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    if (inet_pton(AF_INET, hostname, &server.sin_addr) <= 0) {
        perror("client: inet_pton");
        close(sock_fd);
        exit(1);
    }

    // connection to server
    // Attempt to connect to the server in a loop
    while (1) {
        sock_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_fd < 0) {
            perror("client: socket");
            exit(1);
        }

        if (connect(sock_fd, (struct sockaddr *)&server, sizeof(server)) == 0) {
            // Connection successful
            snprintf(strbuf, 1000, "Connected to server at %s:%d\n", hostname, port);
            // display_message(strbuf);
            break;
        }

        // Connection failed, close the socket and retry
        close(sock_fd);
        // display_message("Server is not available. Retrying in 2 seconds...\n");
        sleep(2); // Wait before retrying
    }

    // reading messages
    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(STDIN_FILENO, &fdset);
    FD_SET(sock_fd, &fdset);
    int maxfd;
    if (sock_fd > STDIN_FILENO) {
        maxfd = sock_fd;
    } else {
        maxfd = STDIN_FILENO;
    }

    // // set up signal handler for SIGINT, to handle Ctrl+C
    // struct sigaction sa_sigint;
    // memset(&sa_sigint, 0, sizeof(sa_sigint));
    // sa_sigint.sa_handler = SIG_IGN;
    // sa_sigint.sa_flags = 0;
    // sigemptyset(&sa_sigint.sa_mask);
    // sigaction(SIGINT, &sa_sigint, NULL);
    // // set up signal handler for SIGPIPE, to handle broken pipe
    // struct sigaction sa_sigpipe;
    // memset(&sa_sigpipe, 0, sizeof(sa_sigpipe));
    // sa_sigpipe.sa_handler = SIG_IGN;
    // sa_sigpipe.sa_flags = 0;
    // sigemptyset(&sa_sigpipe.sa_mask);
    // sigaction(SIGPIPE, &sa_sigpipe, NULL);

    char user_buf[MAX_USER_MSG + 2];
    while (1) {
        fd_set tmp_fds = fdset;
        if (select(maxfd + 1, &tmp_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            break;
        }

        // handle input from stdin
        if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {
            if (fgets(user_buf, MAX_USER_MSG, stdin) == NULL) {
                display_message("Disconnected from server.\n");
                break;
            }

            // connection command call
            if (strncmp(user_buf, "\\connected", 10) == 0) {
                user_buf[strlen(user_buf) - 1] = '\r';
                user_buf[strlen(user_buf)] = '\n';
                user_buf[strlen(user_buf) + 1] = '\0';
            } else {
                // replace new line with network variation
                size_t len = strlen(user_buf);
                if (len > 0 && user_buf[len - 1] == '\n') {
                    user_buf[len - 1] = '\r';
                    user_buf[len] = '\n';
                    user_buf[len + 1] = '\0';
                }
            }

            // send to the server
            if (write_to_socket(sock_fd, user_buf, strlen(user_buf))) {
                display_error("Error sending message to server", "");
                break;
            }
        }

        // handle from the server.
        if (FD_ISSET(sock_fd, &tmp_fds)) {
            int server_closed = read_from_socket(sock_fd, buf, &inbuf);
            if (server_closed == -1) {
                display_error("Error reading from server", "");
                break;
            } else if (server_closed == 1) {
                display_message("Server closed the connection.\n");
                break;
            }
            char *msg;
            while (get_message(&msg, buf, &inbuf) == 0) {
                snprintf(strbuf, 1000, "%s\n", msg);
                display_message(strbuf);
                // display_message("%s\n", msg);
                free(msg);
            }
        }
    }

    close(sock_fd);
}
// Server
// code directly taken from w10's lab.
// static int sigint_received = 0;

static int unique_id = 1;

// static void sigint_handler(int code) {
//     (void) code;
//     sigint_received = 1;
// }

/*
 * Wait for and accept a new connection.
 * Return -1 if the accept call failed.
 */
int accept_connection(int fd, struct client_sock **clients) {
    struct sockaddr_in peer;
    unsigned int peer_len = sizeof(peer);
    peer.sin_family = AF_INET;

    int num_clients = 0;
    struct client_sock *curr = *clients;
    while (curr != NULL && num_clients < MAX_CONNECTIONS && curr->next != NULL) {
        curr = curr->next;
        num_clients++;
    }

    int client_fd = accept(fd, (struct sockaddr *)&peer, &peer_len);
    
    if (client_fd < 0) {
        perror("server: accept");
        close(fd);
        exit(1);
    }

    if (num_clients == MAX_CONNECTIONS) {
        close(client_fd);
        return -1;
    }

    struct client_sock *newclient = malloc(sizeof(struct client_sock));
    newclient->sock_fd = client_fd;
    newclient->inbuf = newclient->state = 0;
    newclient->id = unique_id++;
    newclient->next = NULL;
    memset(newclient->buf, 0, BUF_SIZE);
    // need to get the ip address of the client, and assign it to the client.
    if (inet_ntop(AF_INET, &peer.sin_addr, newclient->ip_address, sizeof(newclient->ip_address)) == NULL) {
        perror("inet_ntop");
        strncpy(newclient->ip_address, "\0", INET_ADDRSTRLEN);
    }



    if (*clients == NULL) {
        *clients = newclient;
    }
    else {
        curr->next = newclient;
    }

    return client_fd;
}

static void clean_exit(struct listen_sock s, struct client_sock *clients, int exit_status) {
    struct client_sock *tmp;
    while (clients) {
        tmp = clients;
        close(tmp->sock_fd);
        clients = clients->next;
        free(tmp);
    }
    close(s.sock_fd);
    free(s.addr);
    exit(exit_status);
}

void start_server(int port) {

    //Code taken directly from w10's lab.
    // setbuf(stdout, NULL);

    /*
     * Turn off SIGPIPE: write() to a socket that is closed on the other
     * end will return -1 with errno set to EPIPE, instead of generating
     * a SIGPIPE signal that terminates the process.
     */
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        perror("signal");
        exit(1);
    }

    // char strbuf[1000];

    // Linked list of clients
    struct client_sock *clients = NULL;

    struct listen_sock s;
    setup_server_socket(&s, port);

    // Set up SIGINT handler: Want to ignore sigint here:
    struct sigaction sa_sigint;
    memset(&sa_sigint, 0, sizeof(sa_sigint));
    sa_sigint.sa_handler = SIG_IGN;
    sa_sigint.sa_flags = 0;
    sigemptyset(&sa_sigint.sa_mask);
    sigaction(SIGINT, &sa_sigint, NULL);

    int exit_status = 0;

    int max_fd = s.sock_fd;

    fd_set all_fds, listen_fds;

    FD_ZERO(&all_fds);
    FD_SET(s.sock_fd, &all_fds);

    do {
        listen_fds = all_fds;
        int nready = select(max_fd + 1, &listen_fds, NULL, NULL, NULL);
        // if (sigint_received) break;
        if (nready == -1) {
            if (errno == EINTR) continue;
            perror("server: select");
            exit_status = 1;
            break;
        }

        /* 
         * If a new client is connecting, create new
         * client_sock struct and add to clients linked list.
         */
        if (FD_ISSET(s.sock_fd, &listen_fds)) {
            int client_fd = accept_connection(s.sock_fd, &clients);
            if (client_fd < 0) {
                display_message("Failed to accept incoming connection.\n");
                continue;
            }
            if (client_fd > max_fd) {
                max_fd = client_fd;
            }
            FD_SET(client_fd, &all_fds);
            // snprintf(strbuf, sizeof(strbuf), "Accepted connection from client %d\n", client_fd);
            // display_message(strbuf);
        }
        // display_message("DEBUG: Incoming connection.\n");
        /*
         * Accept incoming messages from clients,
         * and send to all other connected clients.
         */
        struct client_sock *curr = clients;
        while (curr) {
            if (!FD_ISSET(curr->sock_fd, &listen_fds)) {
                curr = curr->next;
                continue;
            }
            int client_closed = read_from_client(curr);
            char write_buf[BUF_SIZE + 20];
            write_buf[0] = '\0';
            if (client_closed == 2) {
                // null term the buffer at the max message length:
                curr->buf[MAX_USER_MSG] = '\0';
                // Optionally, broadcast the chunk to other clients
                struct client_sock *dest_c = clients;
                while (dest_c) {
                    if (dest_c->ip_address[0] == '\0' || strcmp(dest_c->ip_address, curr->ip_address) != 0) {
                        dest_c = dest_c->next;
                        continue;
                    }    
                    char *formatted_msg = malloc(strlen(curr->buf) + 100);
                    snprintf(formatted_msg, strlen(curr->buf) + 100, "client%d: %s\n", curr->id, curr->buf);
                    display_message(formatted_msg);
                    int ret = write_buf_to_client(dest_c, formatted_msg, strlen(formatted_msg));
                    if (ret == 2) {
                        close(dest_c->sock_fd);
                        FD_CLR(dest_c->sock_fd, &all_fds);
                        assert(remove_client(&dest_c, &clients) == 0);
                        continue;
                    }
                    free(formatted_msg);
                    dest_c = dest_c->next;
                }
            
                // Clear the buffer for the client
                curr->inbuf = 0;
                curr->buf[0] = '\0';
            
                // Skip further processing for this client in this iteration
                continue;
            }

            // If error encountered when receiving data
            if (client_closed == -1) {
                client_closed = 1; // Disconnect the client
            }

            char *msg;
            // display_message("DEBUG: Incoming message with client_closed equal to: ");
            // char msgbuf[100];
            // snprintf(msgbuf, sizeof(msgbuf), "%d\n", client_closed);
            // display_message(msgbuf);
            // Loop through buffer to get complete message(s)
            while (client_closed == 0 && !get_message(&msg, curr->buf, &(curr->inbuf))) {
                // check if the command run was the \\connected command.
                if (strncmp(msg, "\\connected", 10) == 0) {
                    // get the number of clients via iterating thru client_sock:
                    struct client_sock *temp = clients;
                    int num_clients = 0;
                    while (temp) {
                        num_clients++;
                        temp = temp->next;
                    }
                    // print out the number of connected clients, using the unique id:
                    snprintf(write_buf, sizeof(write_buf), "Connected clients: %d", num_clients);
                    // write it to the client that sent the message.
                    int ret = write_buf_to_client(curr, write_buf, strlen(write_buf));
                    if (ret == 2){
                        close(curr->sock_fd);
                        FD_CLR(curr->sock_fd, &all_fds);
                        assert(remove_client(&curr, &clients) == 0);
                        continue;
                    }
                    // don't want to write to everyone else
                    continue;
                }
                // Write the message to the server console, in the format client#: message
                snprintf(write_buf, sizeof(write_buf), "client%d: %s\n", curr->id, msg);
                display_message(write_buf);
                // print_path();
                // display_message("Test Message.");
                // Write the message in the same format as above to all other clients connected to the same hostname on the server.
                struct client_sock *dest_c = clients;
                while (dest_c) {
                    // check for matching ips.
                    // printf("DEBUG: Comparing sender IP %s with recipient IP %s\n", curr->ip_address, dest_c->ip_address);
                    if (dest_c->ip_address[0] == '\0'||strcmp(dest_c->ip_address, curr->ip_address) != 0) {
                        dest_c = dest_c->next;
                        continue;
                    }
                    // need to write to the client the message in the form client#: message
                    char *formatted_msg = malloc(strlen(msg) + 100);
                    snprintf(formatted_msg, strlen(msg) + 100, "client%d: %s", curr->id, msg);
                    // printf("DEBUG: Sending message to client %d: %s\n", dest_c->sock_fd, formatted_msg);
                    int ret = write_buf_to_client(dest_c, formatted_msg, strlen(formatted_msg));
                    if (ret == 0) {
                        // snprintf(strbuf, sizeof(strbuf), "Sent message to client %d\n", dest_c->id);
                        // display_message(strbuf);
                        // print_path();
                    } else {
                        // snprintf(strbuf, sizeof(strbuf), "Failed to send message to client %d\n", dest_c->id);
                        // display_message(strbuf);
                        // print_path();
                        if (ret == 2) {
                            // snprintf(strbuf, sizeof(strbuf), "Client %d disconnected\n", dest_c->sock_fd);
                            // display_message(strbuf);
                            close(dest_c->sock_fd);
                            FD_CLR(dest_c->sock_fd, &all_fds);
                            assert(remove_client(&dest_c, &clients) == 0);
                            continue;
                        }
                    }
                    // free formatted_msg
                    free(formatted_msg);
                    dest_c = dest_c->next;
                }
                free(msg);
            }

            if (client_closed == 1) { // Client disconnected
                FD_CLR(curr->sock_fd, &all_fds);
                close(curr->sock_fd);
                // snprintf(strbuf, sizeof(strbuf), "Client %d disconnected\n", curr->sock_fd);
                // display_message(strbuf);
                assert(remove_client(&curr, &clients) == 0);
            } else {
                curr = curr->next;
            }
        }
    } while (1);

    clean_exit(s, clients, exit_status);
}