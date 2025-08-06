#define _XOPEN_SOURCE 700
#include "builtins.h"
#include "io_helpers.h"
#include "variables.h"
#include "commands.h"
// need to prevent sigint from killing the console:
#include <signal.h>
void sigint_handler(int sig) {
    (void) sig;
    // Forward SIGINT to the foreground process group
    display_message("\n");
    print_path();
}


// You can remove __attribute__((unused)) once argc and argv are used.
int main(__attribute__((unused)) int argc, 
         __attribute__((unused)) char* argv[]) {
    // set output to unbuffered
    // setbuf(stdout, NULL);
    //char *path = "mysh$ "; // TODO Step 1, Uncomment this.
    struct sigaction sa;
    sa.sa_handler = sigint_handler; 
    sa.sa_flags = SA_RESTART;       
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    char input_buf[MAX_STR_LEN + 1];
    input_buf[MAX_STR_LEN] = '\0';
    char *token_arr[MAX_STR_LEN] = {NULL};
    size_t token_count = 0;
    while (1) {
        // Prompt and input tokenization
        //clear of garbage
	    memset(input_buf, 0, sizeof(input_buf));
        for (size_t i = 0; i < token_count; i++) {
            free(token_arr[i]);
            token_arr[i] = NULL;
        }
        // TODO Step 2:
        // Display the prompt via the display_message function.
	    print_path();
        int ret = get_input(input_buf);
        // New version of tokenize_input should now do expansion as it is tokenizing.
        // As such, we do need to change things around.
        token_count = tokenize_input(input_buf, token_arr);
        // Clean exit
        // TODO: The next line has a subtle issue.
        if (ret != -1 && ((token_count == 0 && ret == 0) || (token_arr[0] != NULL && strncmp("exit", token_arr[0], 5) == 0))) {
            for(size_t i = 0 ; i < token_count; i++){
                free(token_arr[i]);
            }
            break;
        }
        // Command execution
        if (token_count >= 1) {
            execute_commands(token_arr, token_count);
        }
    }
    // free the vars, kill the server (if running) and exit
    freeVars();
    close_server();
    return 0;
}
