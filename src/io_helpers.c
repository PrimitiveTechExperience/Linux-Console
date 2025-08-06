#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "variables.h"
#include "io_helpers.h"


// ===== Output helpers =====

/* Prereq: str is a NULL terminated string
 */
void display_message(char *str) {
    write(STDOUT_FILENO, str, strnlen(str, MAX_STR_LEN));
	fflush(stdout);
}


/* Prereq: pre_str, str are NULL terminated string
 */
void display_error(char *pre_str, char *str) {
    write(STDERR_FILENO, pre_str, strnlen(pre_str, MAX_STR_LEN));
    write(STDERR_FILENO, str, strnlen(str, MAX_STR_LEN));
    write(STDERR_FILENO, "\n", 1);
}


// ===== Input tokenizing =====



/* Prereq: in_ptr points to a character buffer of size > MAX_STR_LEN
 * Return: number of bytes read
 */
ssize_t get_input(char *in_ptr) {
    //clear out garbage.
    memset(in_ptr, 0 ,MAX_STR_LEN+1);
    //proceed as normal
    int retval = read(STDIN_FILENO, in_ptr, MAX_STR_LEN+1); // Not a sanitizer issue since in_ptr is allocated as MAX_STR_LEN+1
    int read_len = retval;
    if (retval == -1) {
        read_len = 0;
    }
    // expansion detected, so check where the whitespace is to determine the variable expansion:
    // regular case
    if (read_len > MAX_STR_LEN) {
	//cap it first, then check for expansion to avoid mem issues.
	in_ptr[MAX_STR_LEN] = '\0';
	if(strchr(in_ptr, '$')!=NULL){
		read_len = MAX_STR_LEN;
		retval = MAX_STR_LEN;
		// flush the current input buffer:
		int ch;
		while((ch = getchar())!= '\n' && ch != EOF);
		return retval;
	}
	// otherwise proceed as usual.
        read_len = 0;
        retval = -1;
        write(STDERR_FILENO, "ERROR: input line too long\n", strlen("ERROR: input line too long\n"));
        int junk = 0;
        while((junk = getchar()) != EOF && junk != '\n');
    }
    in_ptr[read_len] = '\0';
    return retval;
}

/* Prereq: in_ptr is a string, tokens is of size >= len(in_ptr)
 * Warning: in_ptr is modified
 * Return: number of tokens.
 */
size_t tokenize_input(char *in_ptr, char **tokens) {
    // TODO: Remove unused attribute
    // clear out garbage
    memset(tokens, 0, sizeof(char)*(MAX_STR_LEN+1));
    // Prep String saves for strtok_r
    char *inStrPtrSave = NULL;
    char *expPtrSave = NULL;
    const char expDelim[] = "$";
    // Strings
    char *exp_ptr = "";
    char *cmd_ptr = strtok_r (in_ptr, DELIMITERS, &inStrPtrSave);
    char *tempStr;

    // int skip_count = 0;
    size_t token_count = 0;
    // before doing the tokenization, check for longer than size:
    if(cmd_ptr != NULL && strlen(cmd_ptr) > MAX_STR_LEN){
	    cmd_ptr[MAX_STR_LEN] = '\0';
    }
    // because it is easier to expand strings during tokenization, we do it in here:
    while (cmd_ptr != NULL && token_count < MAX_STR_LEN) {  // TODO: Fix this
        // TODO: Fix this
	// Check for dollar signs
	if(strchr(cmd_ptr, '$') != NULL){
		// need a sanitized version of the string pre strtok.
		char *sanitized = strndup(cmd_ptr, strlen(cmd_ptr));
		exp_ptr = strtok_r(cmd_ptr, expDelim, &expPtrSave);
    	char *cmdExp = malloc(sizeof(char)*129);
		cmdExp[0] = '\0';
		// Track the number of expansions and dollar signs.
		int dollar = 0;
		int expansion = 0;
		// To track the number of dollar signs, we need to do a simple thing:
		// tracing the number of dollar signs can be done with a for loop,
		// check if we land on either a dollar sign, or a non dollar sign.
		// We will assume that the dollar sign starts first. If not, then
		// that indicates a prefix.
		// First, calculate number of dollar signss
		for(size_t i = 0; i < strlen(sanitized); i++){
			if(sanitized[i] == '$'){
				dollar++;
			}
		}
		if(strncmp(sanitized, "$", 1) != 0){
			//non expansion first, so append the first part
			strncat(cmdExp, exp_ptr, strlen(exp_ptr));
			exp_ptr = strtok_r(NULL, expDelim, &expPtrSave);
			//printf("%s\n", exp_ptr);
		}
		for(size_t i = 0; i < strlen(sanitized)-1; i++){
			//printf("char %ld: %c, next one: %c\n", i, cmd_ptr[i], cmd_ptr[i+1]);
			// check for dollar sign:
			if(sanitized[i] == '$' && sanitized[i+1] == '$'){
				//double dollar sign indicates that there is no expansion, continue as usual.
				if(strlen(cmdExp) > MAX_STR_LEN){
					break;
				}
				strncat(cmdExp, "$", strlen("$") + 1);
			}else if (sanitized[i] == '$' && sanitized[i+1] != '$'){
				//dollar sign followed by something that is not a dollar sign indicates expansion
				//get the variable from the expansion
				//printf("\n\n\n\n\nhi\n");
				tempStr = strndup(getVar(exp_ptr), strlen(getVar(exp_ptr)));
				if(strlen(cmdExp) + strlen(tempStr) > MAX_STR_LEN){
					// concatenate up to the end and stop.
					strncat(cmdExp, tempStr, MAX_STR_LEN-strlen(cmdExp)-1);
					break;
				}
				//concatenate normally
				strncat(cmdExp, tempStr, strlen(tempStr));
				// skip_count = 0;
				exp_ptr = strtok_r(NULL, expDelim, &expPtrSave);
				//free tempStr
				free(tempStr);
				// add one to the expansion count.
				expansion++;
			}
		}
		//before adding, we need to ensure that the string is indeed not overflowing:
		//if(strlen(tokens) + strlen(cmdExp) > MAX_STR_LEN){
		//	cmdExp[MAX_STR_LEN - strlen(tokens)] = '\0'
		//}

		// do a quick check for the length to ensure that there are no dollar signs missing, notably from the end:
		if((dollar - expansion) && strlen(cmdExp) < MAX_STR_LEN){
			// should be zero, if this runs then usually indicates a missing dollar sign at the end.
			strncat(cmdExp, "$", strlen("$")+1);
		}
		//printf("cmd line expanded: %s\n", cmdExp);
		tokens[token_count] = cmdExp;
		// free the sanitized duplicate
		free(sanitized);
	}else{
		// No expansion means continue as usual.
		tokens[token_count] = strndup(cmd_ptr, strlen(cmd_ptr));
	}
	cmd_ptr = strtok_r(NULL, DELIMITERS, &inStrPtrSave);
	token_count += 1;
    }
    tokens[token_count] = NULL;
    return token_count;
}


