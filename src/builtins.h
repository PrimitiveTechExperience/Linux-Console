#ifndef __BUILTINS_H__
#define __BUILTINS_H__

#include <unistd.h>


/* Type for builtin handling functions
 * Input: Array of tokens
 * Return: >=0 on success and -1 on error
 */
typedef ssize_t (*bn_ptr)(char **);
ssize_t bn_echo(char **tokens);
ssize_t bn_cd(char **tokens);
ssize_t bn_cat(char **tokens);
ssize_t bn_wc(char **tokens);
ssize_t bn_ls(char **tokens);
ssize_t bn_ps(char **tokens);
ssize_t bn_kill(char **tokens);
ssize_t bn_start_server(char **tokens);
ssize_t bn_close_server(char **tokens);
ssize_t bn_start_client(char **tokens);
ssize_t bn_send(char **tokens);

char * decode_variable(char * str);
void print_path(void);
/* Return: index of builtin or -1 if cmd doesn't match a builtin
 */
bn_ptr check_builtin(const char *cmd);


/* BUILTINS and BUILTINS_FN are parallel arrays of length BUILTINS_COUNT
 */
static const char * const BUILTINS[] = {"echo", "cd", "cat", "wc", "ls", "ps", "kill", "start-server", "close-server", "start-client", "send"}; // Extra null element for 'non-builtin'
static const bn_ptr BUILTINS_FN[] = {bn_echo, bn_cd, bn_cat, bn_wc, bn_ls, bn_ps, bn_kill, bn_start_server, bn_close_server, bn_start_client, bn_send, NULL};    // Extra null element for 'non-builtin'
static const ssize_t BUILTINS_COUNT = sizeof(BUILTINS) / sizeof(char *);

/* Just a cleanup function for servers (prevents random servers bugging out test cases)
*/
void close_server(void);
#endif