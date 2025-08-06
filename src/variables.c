# include "variables.h"
# include <stdlib.h>
# include <string.h>
# include <stdio.h>
typedef struct variable{
	char *name;
	char *value;
	struct variable *nextVar;
} variable;

variable *varList;
/*
 * Gets a full variable expansion's Value.
 */
int getVarLength(char *str){
	int strLength=0;
	char *currVar = strtok(str, "$");
	// printf("%s\n", getVar(currVar));
	while(currVar != NULL){
		// printf("%s\n", getVar(currVar));
		strLength += strlen(getVar(currVar));
		currVar = strtok(NULL, "$");
	}
	return strLength;
}
/*
 * Decodes a variable for commands/output.
 */

char * decode_variable(char *str){
	char *tempStr = strdup(str);
	int strLength = getVarLength(tempStr);
	free(tempStr);
	// printf("string length: %d\n", strLength);
	// Allocate a string for use:
	char *com = malloc(strLength + 1);
	com[0]='\0';
	char *v;
	// char *prefix = getBefore(str);
	char *currVar = strtok(str, "$");
	//printf("%s\n", prefix);
	//Check for fencepost issue: if there is stuff before $
	//printf("%s\n", currVar);
	// if(strncmp(currVar, prefix, strlen(currVar))){
	// 	strncat(com, currVar, strlen(currVar));
	//	currVar = strtok(str, "$");
	//}
	while(currVar!=NULL){
		v = getVar(currVar);
		//printf("string: %s\n", currVar);
		strncat(com, v, strlen(v));
		currVar = strtok(NULL, "$");
	}
	return com;
}	
variable * createVar(char *name, char *value, variable *nextVar){
	variable *v = malloc(sizeof(variable));
	// printf("\n%s\n", name);
	v->name = malloc(strlen(name)+1);
	strncpy(v->name, name, strlen(name));
	v->name[strlen(name)] = '\0';
	// check for variable expansion.
	char *varV;
	if(strchr(value, '$') != NULL){
		varV = decode_variable(value);
		v->value = strndup(varV, strlen(varV));
	}else{
		v->value= malloc(strlen(value)+1);
		v->value[strlen(value)] = '\0';
		strncpy(v->value, value, strlen(value));
	}
	v->nextVar = nextVar;
	return v;
}

char *getBefore(char *str){
	char *dPos = strchr(str, '$');
	if (dPos == NULL){
		return strdup(""); // no $ indicates decode_var handles everything.
	}
	size_t prefix_len = dPos - str;
	char *prefix = malloc(prefix_len + 1);
	strncpy(prefix, str, prefix_len);
	prefix[prefix_len] = '\0';
	return prefix;
}


void updateVar(char *name, char *value){
	// Null root.
	if(varList == NULL){
		varList = createVar(name, value, varList);
		return ;
	}
	char *varV;
	//printf("\nname: %s\nvalue: %s\n", name, value);
	variable *temp = varList;
	// iterate thru var chain.
	while(temp != NULL){
		// Find matching string
		if((strlen(temp->name) == strlen(name))&&!strncmp(temp->name, name, strlen(name))){
			// check for variable expansion
			if(strchr(value, '$') != NULL){
				varV = decode_variable(value);	
			}else{
				varV = value;
			}	
			free(temp->value);
			temp->value = strndup(varV, strlen(varV));
			// printf("\nmatach found: %s\n", temp->value);
			return;
		}
		temp = temp->nextVar;
	}
	// variable doesn't exist, so add it
	varList = createVar(name, value, varList);
}

char * getVar(char *name){
	// Want to check if the tree is empty:
	
	variable *currVar = varList;
	// printf("%s\n", name);
	if(currVar == NULL){
		return "";
	}
	//printf("\nname:%s\nvalue:%s\n", currVar->name, currVar->value);
	while(currVar != NULL){
		// Check for match
		// printf("%s\n", currVar->name);		
		// printf("%s\n", name);
		// printf("%lu\n", strlen(currVar->name));		
		// printf("%lu\n", strlen(name));	
		if((strlen(currVar->name) == strlen(name))&&!(strncmp(currVar->name, name, strlen(name)))){
			return currVar->value;
		}
		currVar = currVar->nextVar;
	}
	// Did not find a match, so return empty
	return "";
}

void freeVars(){
	if(varList == NULL){
		return;
	}
	if(varList->nextVar == NULL){
		free(varList->name);
		free(varList->value);
		free(varList);
		return;
	}
	variable *nextVar = varList->nextVar;
	variable *currVar = varList;
	while (nextVar!=NULL){
		free(currVar->name);
		free(currVar->value);
		free(currVar);
		currVar = nextVar;
		nextVar = currVar->nextVar;
	}
	free(currVar->name);
	free(currVar->value);
	free(currVar);
}

