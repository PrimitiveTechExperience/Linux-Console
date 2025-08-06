#ifndef __VARIABLES_H__
#define __VARIABLES_H__

typedef struct variable variable;

// Creates a new variable with the name <name> and value
// <value.
variable * createVar(char *name, char *value, variable *nextVar);
// Update/Create the variable with name <name> with the value
// <value>, at the variable list rooted at <root>.
void updateVar(char *name, char *value);
char * getVar(char *name);
void freeVars();
int getVarLength(char *str);
char * decode_variable(char *str);
char * getBefore(char *str);

#endif
