
/*
 * CS-252
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [> filename]
 *
 * you must extend it to understand the complete shell grammar
 *
 */

%error-verbose

%token	<string_val> WORD 

%token  <string_val> SUBSHELLCOMMAND

%token 	NOTOKEN GREAT NEWLINE PIPE LESS GREATAMPERSAND GREATGREAT GREATGREATAMPERSAND AMPERSAND 

%union	{
		char   *string_val;
	}

%{
//#define yylex yylex

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>   // Added by Himal
#include <sys/wait.h>   // Added by Himal
#include <string.h>
#include <signal.h>
#include <fcntl.h>      // Added by Himal
#include <unistd.h>     // Added by Himal
#include <pwd.h>

#include "command.h"
void yyerror(const char * s);
void expandWildcard(char* prefix, char* suffix);
int compare_function(const void *name1, const void *name2);
int yylex();
#define MAXFILENAME 10000

char** array;
int arrayCounter;
int maxSize;
%}

%%

goal:	
	commands
	;

commands: 
	command
	| commands command 
	;

command: 
	 simple_command
        ;

simple_command:
	 pipe_list iomodifier_opt_list background_opt NEWLINE {
		//printf("   Yacc: Execute command\n");
		Command::_currentCommand.execute();
	}
	| NEWLINE 
	| error NEWLINE { yyerrok; }
	;

pipe_list:
	pipe_list PIPE command_and_args
	| command_and_args
	;

command_and_args:
	command_word arg_list {
		Command::_currentCommand.
			insertSimpleCommand( Command::_currentSimpleCommand );
	}
	;

arg_list:
	arg_list argument
	| /* can be empty */
	;

argument:
	WORD {
                //printf("   Yacc: insert argument \"%s\"\n", $1);
		if( strchr($1,'*') != NULL || strchr($1,'?') != NULL){
			maxSize = 50;
			array = (char **)malloc(sizeof(char*)* maxSize);;
			arrayCounter = 0;	
			expandWildcard(NULL, strdup($1));
			qsort(array, arrayCounter, sizeof(char*), compare_function);
			for(int i = 0; i < arrayCounter; i++){
				//printf("%s\n", array[i]);
				Command::_currentSimpleCommand->insertArgument(strdup(array[i]));
			}
			free(array);
		}else{
			Command::_currentSimpleCommand->insertArgument( strdup($1) );\
		}
	} 
	| SUBSHELLCOMMAND {
		
		//printf("HELLO\n");
		int outpipe[2];
		pipe(outpipe);
		
		int pid;
		char * argument = $1;
		//Command::Command * c = new Command;
		struct Command  c = Command();
		if((pid = fork()) == 0){
			c._currentSimpleCommand = new SimpleCommand();
			argument++;
			
			argument[strlen(argument)-1] = ' ';
			argument[strlen(argument)] = '\0';
			char * tmp;
			//printf("%s\n", argument);
			c._currentSimpleCommand->insertArgument(strtok(strdup(argument), " "));
			while((tmp = strtok(NULL," ")) !=NULL){
				c._currentSimpleCommand->insertArgument(strdup(tmp));
				//printf("%s\n", tmp);
			}
			
			dup2(outpipe[1],1);
			close(outpipe[1]);

			c._currentCommand.insertSimpleCommand(c._currentSimpleCommand);
			c._sub = 0;

			c._currentCommand.execute();

			_exit(0);
		}
			
		if(pid > 0 && waitpid(pid,0,0)){
			char * buffer = (char*)calloc(sizeof(char), 5000);
			read(outpipe[0], buffer, 5000);
			if(isatty(0)){
				buffer[strlen(buffer) -8] = ' '; 
				buffer[strlen(buffer) -9] = '\0';
			}
			Command::_currentSimpleCommand->insertArgument(strtok(strdup(buffer), "\n"));
			char * tmp;
			while((tmp = strtok(NULL, "\n")) != NULL){
				Command::_currentSimpleCommand->insertArgument(strdup(tmp));
			}
		}
		close(outpipe[0]);

	}
	;

command_word:
	WORD {
               	//printf("   Yacc: insert command \"%s\"\n", $1);
				       
	       	Command::_currentSimpleCommand = new SimpleCommand();
	       	Command::_currentSimpleCommand->insertArgument( strdup($1) );
	}
	;



iomodifier_opt_list:
	iomodifier_opt_list iomodifier_opt
	| iomodifier_opt
	;

background_opt:
	AMPERSAND {
		Command::_currentCommand._background = 1;
	}
	| /* empty */
	;

iomodifier_opt:
	GREAT WORD {
		//printf("   Yacc: insert output \"%s\"\n", $2);
		if (Command::_currentCommand._outFile == 0){
			Command::_currentCommand._outFile = strdup($2);
		} 
		else{
			printf("Ambiguous output redirect");
			exit(1);
		}
	}
	| LESS WORD {
		//printf("   Yacc: insert input \"%s\"\n", $2);
               if(Command::_currentCommand._inputFile == 0){
	       	 	Command::_currentCommand._inputFile = strdup($2);
	       }
	       else{
			printf("Ambiguous input file");
			exit(1);
	       }
	}
	| GREATAMPERSAND WORD {
		//printf("   Yacc: insert output and error \"%s\"\n", $2);
		if(Command::_currentCommand._outFile == 0){
			Command::_currentCommand._outFile = strdup($2);
			Command::_currentCommand._errFile = strdup($2);
		} else{	
			printf("Ambiguous output redirect");
			exit(1);
		}
	}
	| GREATGREAT WORD {
		//printf("   Yacc: insert output \"%s\"\n", $2);
                if (Command::_currentCommand._outFile == 0){
			Command::_currentCommand._outFile = strdup($2);
			Command::_currentCommand._append = 1;
		}
		else{			
			printf("Ambiguous output redirect");
			exit(1);
		}
	}
	| GREATGREATAMPERSAND WORD {
		//printf("   Yacc: insert output and error \"%s\"\n", $2);
                if (Command::_currentCommand._outFile == 0){
			Command::_currentCommand._outFile = strdup($2);
			Command::_currentCommand._errFile = strdup($2);
			Command::_currentCommand._append = 1;
		}
		else{
			printf("Ambiguous output redirect");
			exit(1);
		}
	}
	| /* can be empty */ 
	;



%%

#include <stdlib.h>
#include <regex.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <dirent.h>

int compare_function(const void *name1, const void *name2){
	return strcmp(*(const char **)name1, *(const char **)name2);
}

void
yyerror(const char * s)
{
	fprintf(stderr,"%s", s);
}

void 
expandWildcard(char * prefix, char * suffix){
	if(suffix[0] == '\0'){
		//Command::_currentSimpleCommand->insertArgument(strdup(prefix));
		if(arrayCounter == maxSize-1){
			maxSize = maxSize *2;
			array = (char**)realloc(array, sizeof(char*) * maxSize);
		}
		array[arrayCounter] = strdup(prefix);
		arrayCounter++;
		return;
	}
	char * s = strchr(suffix,'/');
	char component[MAXFILENAME];
	if(s != NULL){
		if(s-suffix != 0){
			strncpy(component, suffix, s-suffix);
			component[strlen(suffix)-strlen(s)] = '\0';
		}else{
			component[0] = '\0';
		}
		suffix = s+1;
	}else{
		strcpy(component, suffix);
		suffix = suffix + strlen(suffix);
	}
	char newPrefix[MAXFILENAME];
	
	if(strchr(component,'*') == NULL && strchr(component,'?') == NULL){
		if(prefix == NULL && component[0] != '\0'){
			sprintf(newPrefix, "%s", component);
		}else{
			sprintf(newPrefix,"%s/%s",prefix,component);
		}
		if(component[0] == '\0'){
			char * t = (char*)"";
			expandWildcard(t,suffix);	
		}else{
			expandWildcard(newPrefix,suffix);
		}
		return;
	}

	char * exp = (char*)malloc(sizeof(char) * (2*(strlen(component)+10)));
	char * a = component;
	char * r = exp;
	
	*r = '^'; r++;
	while(*a != '\0'){
		if (*a == '*') { *r = '.'; r++; *r = '*'; r++;} 
		else if (*a == '?') { *r = '.'; r++;}
		else if (*a == '.') { *r='\\'; r++; *r='.'; r++;}
		else { *r=*a; r++;}
		a++;
	}
	*r='$'; r++; *r='\0';

	regex_t tmp;
	//printf("%s\n",exp);	
	int check = regcomp(&tmp, exp, 0);
	
	if(check > 0){
		perror("compile");
		return;
	}

	DIR * dir;
	//printf("hello\n");
	if(prefix == NULL){
		//printf("Hello\n");
		dir = opendir(".\0");
	} else if(strcmp(prefix, "") == 0){
		dir = opendir("/\0");
	} else {
		//printf("hello: %s\n",prefix);
		dir = opendir(prefix);	
	}


	//printf("world\n");
	if(dir == NULL){
		//printf("here?\n");
		//perror("opendir FAILED");
		return;
	}	
	struct dirent * ent;

	while((ent = readdir(dir)) != NULL){
		//printf("%s\n", ent->d_name);
		if(regexec(&tmp,ent->d_name,0,NULL,0) == 0){
			//printf("%s\n",ent->d_name);
			//printf("hello\n");
			if(ent->d_name[0] == '.'){
				if(component[0] == '.'){
					if(prefix == NULL){	
						sprintf(newPrefix, "%s", ent->d_name);
					}else{
						sprintf(newPrefix, "%s/%s", prefix, ent->d_name);
					}
					expandWildcard(newPrefix,suffix);
				}
			}else{
				if(component[0] != '.'){
					//printf("hello\n");
					if(prefix == NULL){
						sprintf(newPrefix, "%s", ent->d_name);
					}else{
						sprintf(newPrefix, "%s/%s", prefix, ent->d_name);
					}
					expandWildcard(newPrefix,suffix);
				}
			}
		}
	}	
	closedir(dir);

}


#if 0
main()
{
	yyparse();
}
#endif
