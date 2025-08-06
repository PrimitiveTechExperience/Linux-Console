#define _GNU_SOURCE

#include "builtins.h"
#include "io_helpers.h"
#include "variables.h"
#include "commands.h"


#define MAX_DEPTH 9999
// We'll make an assumption that the depth of the directory is at most 9999.
// Directory depth is unlikely to be that deep.
char CURR_WORKING_DIR[4096] = "mysh$ ";
char *filter;
// Server stuff, need to track if the server is running, and furthermore, its pid.
static pid_t server_pid = -1;
static int server_running = 0;
// ===== Helper Functions =====
static void sigchld_handler(int sig) {
    (void)sig;
    int status;
    pid_t pid;

    // Reap all exited child processes
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            server_running = 0; // Mark the server as not running
            // display_message("Server process has exited.\n");
        }
    }
}
/*
 * Gets the length of a variable expansion
 */
/*
 * Returns out a full variable expansion (when it is a NOT singular $ sign).
 */
void bn_echo_expansion(char *str){
	char *currVar;
	//to expand a thing, we use the fence post problem sol'n
	if((currVar = strtok(str, "$"))!=NULL){
		if(strlen(currVar) == 0){
			display_message("$");
		}else{
			display_message(getVar(currVar));
		}
	}
	// Proceed as usual
	while((currVar = strtok(NULL, "$")) != NULL){
		// Case for singular $ sign.
		if(strlen(currVar) == 0){
			display_message("$");
			continue;
		}
		display_message(getVar(currVar));
	}
}
/*
	Return whether or not a filename contains the specified filter.	
*/
int filter_check(const struct dirent *entry){
	// check if the filter is NULL, if so, then we return 1.
	if(filter == NULL){
		return 1;
	}
	// printf("Filtering: %s in %s, status: %s\n", filter, entry->d_name, strstr(entry->d_name, filter));
	// check if the filter is in the entry name.	
	if(strstr(entry->d_name, filter) != NULL){
		
		return 1;
	}
	return 0;
}
/*
 Aids in the expansion of directories (provided a depth is provided).
*/
int traverse_dir_depth(char *dir, int depth, int maxDepth){
	// base case: maxdepth of zero displays the current directory's name:
	if(maxDepth == 0){
		display_message(dir);
		display_message("\n");
		return 0;
	}
	// base case: dir is actually not a directory, but a file.
	// if (opendir(dir) == NULL){
	// 	display_message(dir);
	// 	display_message("\n");
	// 	return 0;
	// }
	// base case: we have reached the max depth.
	if(depth >= maxDepth){
		struct dirent **nameList;
		// filter_check has a NULL check, so we don't need to worry about that.
		int n = scandir(dir, &nameList, filter_check, NULL), err = 0;
		if(n < 0){
			display_error("ERROR: Invalid path: ", dir);
			// free the nameList and its contents
			for(int i = 0; i < n; i++){
				free(nameList[i]);
			}
			free(nameList);
			return -1;
		}
		for(int i = 0; i < n; i++){
			// print the name of the file.
			display_message(nameList[i]->d_name);
			display_message("\n");
			// free the nameList[i] once done.
			free(nameList[i]);
		}
		// free the nameList once done.
		free(nameList);
		return err;
	}else{ // recursive case: we have not reached the max depth.
		// List all files in the directory.
		struct dirent **nameList;
		int n = scandir(dir, &nameList, filter_check, NULL), err = 0;
		if(n < 0){
			display_error("ERROR: Cannot open directory: ", dir);
			// free the nameList and its contents
			for(int i = 0; i < n; i++){
				free(nameList[i]);
			}
			free(nameList);
			return -1;
		}
		for(int i = 0; i < n; i++){
			display_message(nameList[i]->d_name);
			display_message("\n");
			free(nameList[i]);
		}
		free(nameList);
		// Recurse on all directories: Reset nameList
		n = scandir(dir, &nameList, NULL, NULL);
		for(int i = 0; i < n; i++){
			if(nameList[i]->d_type == DT_DIR){
				// skip the parent and current directories:
			if((strlen(nameList[i]->d_name) == 1 && !strncmp(nameList[i]->d_name, ".", strlen(".")))
				|| (strlen(nameList[i]->d_name) == 2 && !strncmp(nameList[i]->d_name, "..", strlen("..")))){
					free(nameList[i]);
					continue;
			}
				// create the path to the directory.
				char path[4096];
				strncpy(path, dir, strlen(dir));
				strncat(path, "/", 2);
				strncat(path, nameList[i]->d_name, strlen(nameList[i]->d_name));
				err += traverse_dir_depth(path, depth+1, maxDepth);
				// clear char path arr
				memset(path, 0, 4096);
			}
			// free the directories once done, as they are no longer needed.
			free(nameList[i]);
		}
		// Free nameList once done.
		free(nameList);
		return err;
	}
}

// ======== Path Printing =======

void print_path(){
	display_message(CURR_WORKING_DIR);
}


// ====== Command execution =====

/* Return: index of builtin or -1 if cmd doesn't match a builtin
 */
bn_ptr check_builtin(const char *cmd) {
    // NOTE: Change this code as var expansion is now down beforehand.
    ssize_t cmd_num = 0;
    while (cmd_num < BUILTINS_COUNT &&
           strncmp(BUILTINS[cmd_num], cmd, MAX_STR_LEN) != 0) {
        cmd_num += 1;
    }
    return BUILTINS_FN[cmd_num];
}

// ====== Server Cleanup =====

void close_server(){
	if(server_running){
		// send the signal to the server.
		if (kill(server_pid, SIGKILL) == -1) {
			return;
		}
		// mark the server as not running.
		server_running = 0;
		server_pid = -1;
	}
}

// ===== Builtins =====

/* Prereq: tokens is a NULL terminated sequence of strings.
 * Return 0 on success and -1 on error ... but there are no errors on echo. 
 */
ssize_t bn_echo(char **tokens) {
    ssize_t index = 1;

    if (tokens[index] != NULL) {
        // TODO:
        // Implement the echo command
	display_message(tokens[index]);
	index += 1;
    }
    while (tokens[index] != NULL) {
        // TODO:
        // Implement the echo command
	display_message(" ");
	//if(strchr(tokens[index], '$') != NULL && (strlen(tokens[index]) > 1)){
        	//printf("\n\n%s\n\n", tokens[index]);
	//	bn_echo_expansion(tokens[index]);
	//}else{
		display_message(tokens[index]);
	//}
	index += 1;
    }
    display_message("\n");

    return 0;
}

/* Prereq: tokens is a NULL terminated sequence of strings.
 * Return 0 on success and -1 on error ... but there are no errors on echo. 
 */

ssize_t bn_wc(char **tokens){
	ssize_t index = 1;
    FILE *file = NULL;
    if (tokens[index] == NULL) {
        // No input source provided, read from STDIN
        file = stdin;
    } else {
        // multiple paths are provided.
        if (tokens[index + 1] != NULL) {
            display_error("ERROR: Too many arguments: wc takes a single file", "");    
            return -1;
        }
        file = fopen(tokens[index], "r");
        if (file == NULL) {
            display_error("ERROR: Cannot open file: ", tokens[index]);
            return -1;
        }
    }
	if(file == NULL){
		display_error("ERROR: No input source provided", "");
		return -1;
	}
	// set up counters:
	int characters = 0, words = 0, lines = 0;
	// word flag
	int wordD = 0;
	char c = fgetc(file);
	while(c != EOF){
		characters++;
		if(c == '\n'){
			lines++;
		}
		if(!wordD && c != '\n' && c != '\t' && c != '\r' && c != ' '){
			// non whitespace characters are beginning of words.
			wordD = 1;
		}else if(wordD && (c == '\n' || c == '\t' || c == '\r' || c == ' ')){
			// word detected.
			words++;
			wordD = 0;	
		}
		c = fgetc(file);	
	}
	fclose(file);
	//account for EOF.
	//characters++;
	// convert the words, characters, lines counts to strings
	char wordsStr[128], charactersStr[128], linesStr[128];
	sprintf(wordsStr, "%d", words);
	sprintf(charactersStr, "%d", characters);
	sprintf(linesStr, "%d", lines);
	display_message("word count ");
	display_message(wordsStr);
	display_message("\n");
	display_message("character count ");
	display_message(charactersStr);
	display_message("\n");
	display_message("newline count ");
	display_message(linesStr);
	display_message("\n");
	return 0;
}


/* Prereq: tokens is a NULL terminated sequence of strings.
 * Return 0 on success and -1 on error ... but there are no errors on echo. 
 */

ssize_t bn_ls(char **tokens){
	ssize_t index = 1;
	char *leftovers;
	int depth = 1, rec = 0, dProv = 0, pProv = 0;
	// Set filter to null:
	filter = NULL;
	char *path = ".";
	// Parse the tokens.
	while(tokens[index] != NULL){
		// display_message("Tokens: ");
		// display_message(tokens[index]);
		// display_message("\n");
		// otherwise, we simply parse the flags.
		if(!strncmp(tokens[index], "--f", strlen("--f"))){
			if(tokens[index+1] == NULL){
				display_error("ERROR: no filter provided", "");
				return -1;
			}
			//check if filter is defined, if so, then check if they match. 
			//throw an error if they don't.
			if(filter != NULL && strlen(filter) == strlen(tokens[index+1])
			 && strncmp(filter, tokens[index+1], strlen(tokens[index+1]))){
				display_error("ERROR: Filters differ: ", tokens[index+1]);
				return -1;
			}
			filter = strndup(tokens[index+1], strlen(tokens[index+1]));
			index++;
		}else if(!strncmp(tokens[index], "--rec", strlen("--rec"))){
			rec = 1;
		}else if(!strncmp(tokens[index], "--d", strlen("--d"))){
			if(tokens[index+1] == NULL){
				display_error("ERROR: no depth provided", "");
				return -1;
			}
			// check if the depth was already provided; if so, then check if they match.
			// throw an error if they don't.
			if(dProv && depth != strtol(tokens[index+1], &leftovers, 10)){
				display_error("ERROR: Depths differ: ", tokens[index+1]);
				return -1;
			}
			depth = strtol(tokens[index+1], &leftovers, 10);
			// never want any leftovers, indicates that the number was not just numbers.
			// Similarly, no negative depths.
			if(strlen(leftovers) > 0 || depth < 0){
				display_error("ERROR: Invalid depth: ", tokens[index+1]);
				return -1;
			}
			// depth was provided:
			dProv = 1;
			index++;
		}else{
			// indicates that a path was provided: check if a path was already provided, and aligns with the current path:
			if(pProv && strncmp(path, tokens[index], strlen(tokens[index])) && strlen(path) != strlen(tokens[index])){
				display_error("ERROR: Too many arguments: ls takes a single path", "");	
				return -1;
			}
			path = tokens[index];
			pProv = 1;
		}
		index++;
	}
	// depth provided and no rec is error:
	if(dProv && !rec){
		display_error("ERROR: Depth provided without recursion", "");
		return -1;
	}
	// depth not provided, rec provided, set the depth to the MAX_DEPTH macro.
	if(!dProv && rec){
		depth = MAX_DEPTH;
	}
	// if the path for some reason gets defaulted to null terminator, change it to working dir:
	if (strlen(path) == 0){
		path = ".";
	}
	// Call directory traversal function.
	int err = traverse_dir_depth(path, 1, depth);
	if(err){
		// display_error("ERROR: Error in directory traversal", "");
		return -1;
	}	
	free(filter);
	// flush output:
	// fflush(stdout);
	return 0;
}

/* Prereq: tokens is a NULL terminated sequence of strings.
 * Return 0 on success and -1 on error ... but there are no errors on echo. 
 */
ssize_t bn_cat(char **tokens){
	ssize_t index = 1;
    FILE *file = NULL;
    if (tokens[index] == NULL) {
        // No input source provided, read from STDIN
        file = stdin;
    } else {
        // multiple paths are provided.
        if (tokens[index + 1] != NULL) {
            display_error("ERROR: Too many arguments: cat takes a single file", "");    
            return -1;
        }
        // check if the file given is a directory.
        DIR *dir = opendir(tokens[index]);    
        if (dir != NULL) {
            display_error("ERROR: Cannot cat a directory: ", tokens[index]);
            closedir(dir);
            return -1;
        }
        file = fopen(tokens[index], "r");
        if (file == NULL) {
            display_error("ERROR: Cannot open file: ", tokens[index]);
            return -1;
        }
    }
	if(file == NULL){
		display_error("ERROR: No input source provided", "");
		return -1;
	}
    char output[128];
    while (fgets(output, sizeof(output), file) != NULL) {
        display_message(output);
    }
    if (file != stdin) {
        fclose(file);
    }
	// flush the output (side note: love it when python reads the wrong thing.)
	fflush(stdout);
    return 0;
}

/* Prereq: tokens is a NULL terminated sequence of strings.
 * Return 0 on success and -1 on error ... but there are no errors on echo. 
 */
ssize_t bn_cd(char **tokens){
	ssize_t index = 1;
	if(tokens[index] == NULL){
		display_error("ERROR:", "cd missing path");
		return -1;
	}
	// multiple paths are provided.
	if(tokens[index + 1] != NULL){
		display_error("ERROR: Too many arguments: cd takes a single path", "");	
		return -1;
	}
	//printf("cd: %s\n strncmp to ....: %d\n", tokens[index], strncmp(tokens[index], "....", strlen("....")));
	if(strlen(tokens[index])==4 && !strncmp(tokens[index], "....", strlen("...."))){
		//surely the best way of doing this.
		if(chdir("..")){
			display_error("ERROR: Invalid Path: ", tokens[index]);
			return -1;
		}
		if(chdir("..")){
			display_error("ERROR: Invalid Path: ", tokens[index]);
			return -1;
		}
		if(chdir("..")){
			display_error("ERROR: Invalid Path: ", tokens[index]);
			return -1;
		}
	}else if(strlen(tokens[index])==3 && !strncmp(tokens[index], "...", strlen("..."))){
		if(chdir("..")){
			display_error("ERROR: Invalid Path: ", tokens[index]);
			return -1;
		}
		if(chdir("..")){
			display_error("ERROR: Invalid Path: ", tokens[index]);
			return -1;
		}
	}else if(chdir(tokens[index])){	
		display_error("ERROR: Invalid Path: ", tokens[index]);
		return -1;
	}
	return 0;
}

ssize_t bn_ps(char **tokens){
	tokens[0] = tokens[0]; // suppress unused variable warning
	return print_background_processes();
}

ssize_t bn_kill(char **tokens){
	ssize_t pindex = 1;
	ssize_t sindex = 2;
	ssize_t pid = -1;
	ssize_t signum = -1;
	char *leftovers;
	// check if correct number of args are provided:
	if (tokens[pindex] == NULL){
		display_error("ERROR: kill takes no less than 1 arg." ,"");
		return -1;
	}
	// check if the current index is the pid (which should only be numbers).
	if(pid != -1 && pid != strtol(tokens[pindex], &leftovers, 10)){
		display_error("ERROR: pid differ: ", tokens[pindex]);
		return -1;
	}
	if(pid == -1){
		pid = strtol(tokens[pindex], &leftovers, 10);
	}
	// never want any leftovers, indicates that the number was not just numbers.
	// Similarly, no negative pids.
	if(strlen(leftovers) > 0 || pid < 0){
		display_error("ERROR: Invalid pid: ", tokens[pindex]);
		return -1;
	}
	if(tokens[sindex] != NULL){
	// check if the current index is the signal number (which should only be numbers).
		if(signum != -1 && signum != strtol(tokens[sindex], &leftovers, 10)){
			display_error("ERROR: signum differ: ", tokens[sindex]);
			return -1;
		}
		if(signum == -1){
			signum = strtol(tokens[sindex], &leftovers, 10);
		}
		// never want any leftovers, indicates that the number was not just numbers.
		// Similarly, no negative pids.
		if(strlen(leftovers) > 0 || signum <= 0 || signum >= NSIG){
			display_error("ERROR: Invalid signal specified: ", tokens[sindex]);
			return -1;
		}
		// check for invalid signum:
		if (signum <= 0 || signum >= NSIG) {
			display_error("ERROR: Invalid signal specified: ", tokens[sindex]);
			return -1;
		}
	}else{
		//set signum to sigterm
		signum = SIGTERM;
	}
	// Attempt to send the signal
    if (kill(pid, signum) == -1) {
        display_error("ERROR: The process does not exist or cannot be signaled: ", tokens[1]);
        return -1;
    }
	return 0;
}

ssize_t bn_start_server(char **tokens) {
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sigchld_handler;
	sigaction(SIGCHLD, &sa, NULL);

    int index = 1;

    if (tokens[index] == NULL) {
        display_error("ERROR: No port provided", "");
        return -1;
    }

    if (tokens[index + 1] != NULL) {
        display_error("ERROR: Too many arguments: start_server takes a single port", "");
        return -1;
    }

    if (server_running) {
        display_error("ERROR: Server is already running", "");
        return -1;
    }

    int port = atoi(tokens[index]);
    if (port <= 0) {
        display_error("ERROR: Invalid port number", "");
        return -1;
    }
	// check if the port is already in use by another server instance:
	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd < 0) {
		display_error("ERROR: Failed to create socket", "");
		return -1;
	}
	struct sockaddr_in server;
	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = INADDR_ANY;
	if (bind(sock_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
		display_error("ERROR: Port is already in use", "");
		close(sock_fd);
		return -1;
	}
	// close the socket after binding to check if the port is in use.
	close(sock_fd);

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    }
	//child process: start server.
    if (pid == 0) {
        start_server(port);
        exit(0);
    }

    // mark the server as running, update the server pid.
    server_running = 1;
	server_pid = pid;
    // display_message("Server started in the background.\n");

    return 0;
}

ssize_t bn_close_server(char **tokens){
	tokens[0] = tokens[0]; // suppress unused variable warning
	// check if server is running, if not, then display error.
	if (!server_running || server_pid == -1) {
		display_error("ERROR: Server is not running", "");
		return -1;
	}
	// send the signal to the server.
	if (kill(server_pid, SIGTERM) == -1) {
		// Only gets here if something went wrong with the kill.
		if (kill(server_pid, SIGKILL) == -1) {
            display_error("ERROR: Failed to forcefully terminate server", "");
			return -1;
        }
	}
	// mark the server as not running.
	server_running = 0;
	server_pid = -1;
	// display_message("Server closed.\n");
	return 0;
}

ssize_t bn_start_client(char **tokens) {
	//check for provided hostname, port.
    if (tokens[1] == NULL) {
        display_error("ERROR: No port provided", "");
        return -1;
    }

    if (tokens[2] == NULL) {
        display_error("ERROR: No hostname provided", "");
        return -1;
    }
	// check for valid port number.
    int port = atoi(tokens[1]);
    if (port <= 0) {
        display_error("ERROR: Invalid port number", "");
        return -1;
    }
    const char *hostname = tokens[2];
    // start the client
	start_client(port, hostname);
	// if we catch any ctrl d, then we return here
    return 0;
}

ssize_t bn_send(char **tokens){
	// send port host_name msg
	// check for provided hostname, port.
	if (tokens[1] == NULL) {
		display_error("ERROR: No port provided", "");
		return -1;
	}

	if (tokens[2] == NULL) {
		display_error("ERROR: No hostname provided", "");
		return -1;
	}
	// check that the port is a valid number.
	int port = atoi(tokens[1]);
	if (port <= 0) {
		display_error("ERROR: Invalid port number", "");
		return -1;
	}
	const char *hostname = tokens[2];
	// need to send message to the server
	// if msg empty, substitute for newline character.
	char *msg;
	if (tokens[3] == NULL) {
		// to retain consistency with the other case, we need to allocate memory for the newline character.
		msg = malloc(2);
		strcpy(msg, "\n");
	} else {
		// need to concatenate all the tokens past the first 3 tokens.
		int msg_len = 0;
		for (int i = 3; tokens[i] != NULL; i++) {
			msg_len += strlen(tokens[i]) + 1;
		}
		msg = malloc(msg_len + 1);	
	}
	// fencepost problem solution.
	if (tokens[3] != NULL) {
		strcpy(msg, tokens[3]);
	}
	for (int i = 4; tokens[i] != NULL; i++) {
		strcat(msg, " ");
		strcat(msg, tokens[i]);
	}
	//create the socket
	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd < 0) {
		display_error("ERROR: Failed to create socket", "");
        return -1;
	}
	// set up the server address.
	struct sockaddr_in server;
	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);

	// convert hostname to IP address
	if (inet_pton(AF_INET, hostname, &server.sin_addr) <= 0) {
		display_error("ERROR: Invalid hostname or IP address", "");
        close(sock_fd);
        return -1;
	}
	// connect to the server
	if (connect(sock_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
		display_error("ERROR: Failed to connect to server", "");
        close(sock_fd);
        return -1;
	}
	// verify that the server is actually running (so that the message is not written to the socket).
	int error = 0;
	socklen_t len = sizeof(error);
	if (getsockopt(sock_fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0 || error != 0) {
		display_error("ERROR: Failed to connect to server", strerror(error));
		close(sock_fd);
		return -1;
	}
	// add server newline character to the message
	char formatted_msg[BUF_SIZE];
	snprintf(formatted_msg, sizeof(formatted_msg), "%s\r\n", msg);
	// printf("Sending message: %s\n", formatted_msg);
	// send to server.
	int code = write_to_socket(sock_fd, formatted_msg, strlen(formatted_msg));
	// display_message("DEBUG: Sent message to server with code: ");
	// display_message(code == 0 ? "0" : "1");
	// display_message("\n");
	if (code == 1) {
		display_error("ERROR: Failed to send message to server", "");
		close(sock_fd);
		return -1;
	}else if (code == 2){
		display_error("ERROR: Unexpected Disconnection", "");
		close(sock_fd);
		return -1;
	}
	//close the socket and free msg
	close(sock_fd);
	free(msg);
	return 0;
}

