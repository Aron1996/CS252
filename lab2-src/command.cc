
/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>  	// Added by Himal
#include <sys/wait.h>  	// Added by Himal
#include <string.h>
#include <signal.h>
#include <fcntl.h>     	// Added by Himal
#include <unistd.h>	// Added by Himal
#include <pwd.h>	// Added by Himal

#include "command.h"

int *bkgroundPIDs;
int bkgroundPIDsCounter;

extern char **environ;

SimpleCommand::SimpleCommand()
{
	// Creat available space for 5 arguments
	_numberOfAvailableArguments = 5;
	_numberOfArguments = 0;
	_arguments = (char **) malloc( _numberOfAvailableArguments * sizeof( char * ) );
}

void
SimpleCommand::insertArgument( char * argument )
{
	if ( _numberOfAvailableArguments == _numberOfArguments  + 1 ) {
		// Double the available space
		_numberOfAvailableArguments *= 2;
		_arguments = (char **) realloc( _arguments,
				  _numberOfAvailableArguments * sizeof( char * ) );
	}
	
	
	if(argument[0] == '~'){
		if(argument[1] == '\0'){
			//printf("%d\n", getuid());
			struct passwd *pw = getpwuid(getuid());
			const char * directory = pw->pw_dir;
			strcpy(argument,directory);
		}
		else{
			char * tmp = argument;
			int len = strlen(argument);
			char * subdir = (char*)calloc(sizeof(char), (len+20));
			char * back = subdir;
			tmp++;
			while(*tmp != '\0'){
				if(*tmp == '/'){
					break;
				}
				*subdir = *tmp;
				subdir++;
				tmp++;
			}
			--subdir = '\0';
			subdir = back;
			if(*tmp == '\0'){
				struct passwd *pw = getpwnam(subdir);
				const char * directory = pw->pw_dir;
				argument = strdup(directory);
			}else{
				char * t = strcat(getpwnam(subdir)->pw_dir,tmp);
				argument = strdup(t); 
			}
			free(subdir);
		}
	}
	

	char * newArgument = (char*)malloc(sizeof(char) * (strlen(argument) + 200));
	char * globVar = (char*)malloc(sizeof(char) * 50);
	char * globVarVal = (char*)malloc(sizeof(char) * 150);

	int newArgCounter = 0;
	int oldArgCounter = 0;

	while(argument[oldArgCounter] != '\0'){
		if(argument[oldArgCounter] == '\\' && argument[oldArgCounter+1] == '$'){
			oldArgCounter++;
			while(argument[oldArgCounter] != '}'){
				newArgument[newArgCounter] = argument[oldArgCounter];
				newArgCounter++;
				oldArgCounter++;
			}
		}
		else if (argument[oldArgCounter] == '$' && argument[oldArgCounter+1] == '{'){
			oldArgCounter+=2;
			int counter = 0;
			while(argument[oldArgCounter] != '}'){
				globVar[counter] = argument[oldArgCounter];
				counter+=1;
				oldArgCounter+=1;
			}
			oldArgCounter+=1;
			globVar[counter] = '\0';
			globVarVal = getenv(globVar);	
			for(int i = 0; i < strlen(globVarVal); i++){
				newArgument[newArgCounter] = globVarVal[i];
				newArgCounter++;
			}
		}else{
			newArgument[newArgCounter] = argument[oldArgCounter];
			newArgCounter+=1;
			oldArgCounter+=1;
			
		}
	}
	newArgument[newArgCounter] = '\0'; 

	_arguments[ _numberOfArguments ] = newArgument;
	
	// Add NULL argument at the end
	_arguments[ _numberOfArguments + 1] = NULL;
	
	_numberOfArguments++;
}

Command::Command()
{
	// Create available space for one simple command
	_numberOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)
		malloc( _numberOfSimpleCommands * sizeof( SimpleCommand * ) );

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
	_append = 0;
	_sub = 1;
}

void
Command::insertSimpleCommand( SimpleCommand * simpleCommand )
{
	if ( _numberOfAvailableSimpleCommands == _numberOfSimpleCommands ) {
		_numberOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **) realloc( _simpleCommands,
			 _numberOfAvailableSimpleCommands * sizeof( SimpleCommand * ) );
	}
	
	_simpleCommands[ _numberOfSimpleCommands ] = simpleCommand;
	_numberOfSimpleCommands++;
}

void
Command:: clear()
{
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		for ( int j = 0; j < _simpleCommands[ i ]->_numberOfArguments; j ++ ) {
			free ( _simpleCommands[ i ]->_arguments[ j ] );
		}
		
		free ( _simpleCommands[ i ]->_arguments );
		free ( _simpleCommands[ i ] );
	}

	if ( _outFile ) {
		free( _outFile );
	}

	if ( _inputFile ) {
		free( _inputFile );
	}

	if ( _errFile ) {
		free( _errFile );
	}

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
}

void
Command::print()
{
	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  --- ----------------------------------------------------------\n");
	
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		printf("  %-3d ", i );
		for ( int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++ ) {
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[ j ] );
		}
	printf("\n");
	}

	printf( "\n\n" );
	printf( "  Output       Input        Error        Background\n" );
	printf( "  ------------ ------------ ------------ ------------\n" );
	printf( "  %-12s %-12s %-12s %-12s\n", _outFile?_outFile:"default",
		_inputFile?_inputFile:"default", _errFile?_errFile:"default",
		_background?"YES":"NO");
	printf( "\n\n" );
	
}

void
Command::execute()
{
	// Don't do anything if there are no simple commands
	if ( _numberOfSimpleCommands == 0 ) {
		if(_sub)
			prompt();
		return;
	}
	

	if (strcmp(_simpleCommands[0]->_arguments[0], "exit") == 0){
		printf("\nGood Bye!!\n\n");
		exit(0);
	}
	else if(strcmp(_simpleCommands[0]->_arguments[0], "setenv") == 0){	
		setenv(_simpleCommands[0]->_arguments[1],_simpleCommands[0]->_arguments[2],1);
		clear();
		prompt();
		return;
	}
	else if(strcmp(_simpleCommands[0]->_arguments[0], "unsetenv") == 0){
		unsetenv(_simpleCommands[0]->_arguments[1]);
		clear();
		prompt();
		return;
	}	
	else if(strcmp(_simpleCommands[0]->_arguments[0], "cd") == 0 || strcmp(_simpleCommands[0]->_arguments[0], "..") == 0){
		if(_simpleCommands[0]->_arguments[1] == NULL){	
			struct passwd *pw = getpwuid(getuid());
			const char *homedir = pw->pw_dir;
			chdir(homedir);
		}else if (strcmp(_simpleCommands[0]->_arguments[0], "..") == 0){
			chdir("..");
		}
		else{
			chdir(_simpleCommands[0]->_arguments[1]);
		}
		clear();
		prompt();
		return;
	}

	// Print contents of Command data structure
	// This was commented out so that the testall will pass
	//print();

	

	//  in/out vairables
	int tmpin = dup(0);
	int tmpout = dup(1);
	int tmperr = dup(2);

	// initial input
	int fdin;
	int fdout;
	int fderr;

	if (_inputFile){
		fdin = open(_inputFile, O_RDONLY);
	}else{
		fdin  = dup(tmpin);
	}
	
	// For every simple command fork a new process

	int pid; 

	for(int i = 0; i < _numberOfSimpleCommands; i++){
		// For File Redirection
		dup2(fdin,0);
		close(fdin);
		if ( i == _numberOfSimpleCommands - 1){
			if (_outFile){
				if(_append){
					fdout = open(_outFile, O_WRONLY|O_CREAT|O_APPEND, 0700);
				}
				else{
					fdout = open(_outFile, O_WRONLY|O_CREAT|O_TRUNC, 0700);
				}
			}
			else{
				fdout = dup(tmpout);
			}
	                if(_errFile){
			        if(_append){
		        	        fderr = open(_errFile, O_WRONLY|O_CREAT|O_APPEND, 0700 );
		        	}
	                        else{
		                        fderr = open(_errFile, O_WRONLY|O_CREAT|O_TRUNC, 0700);
		                }
	   	        }else{
	 	                 fderr = dup(tmperr);
	                }
	                dup2(fderr,2);
		        close(fderr);
			
		}
		else {
			int fdpipe[2];
			pipe(fdpipe);
			fdout=fdpipe[1];
			fdin=fdpipe[0];
		}
		
		dup2(fdout,1);
		close(fdout);

		pid = fork();
		if ( pid < 0){
			perror("fork");
			exit(2);
		} 
		else if (pid == 0){
			if(strcmp(_simpleCommands[i]->_arguments[0], "printenv") == 0){
				char **p = environ; 
				while(*p != NULL){
					printf("%s\n",*p);
					p++;
				}
				exit(0);
			}
			else{
				execvp(_simpleCommands[i]->_arguments[0], _simpleCommands[i]->_arguments);
				perror("execvp");
				_exit(0);
			}
		}
		
	}	
	
	dup2(tmpin,0);
	dup2(tmpout,1);
	dup2(tmperr,2);
	
	close(tmpin);
	close(tmpout);
	close(tmperr);

	if(!_background){
		waitpid(pid,0,0);
	}else{
		if(bkgroundPIDsCounter >=2048){
			perror("Too many background processes");
		}else{
			bkgroundPIDs[bkgroundPIDsCounter] = pid;
			bkgroundPIDsCounter++;
		}
	}
	// Clear to prepare for next command
	clear();
	
	// Print new prompt
	prompt();
}

// Shell implementation

void
Command::prompt()
{
	if(isatty(0) && _sub){
		char buf[2048];
		getcwd(buf,sizeof(buf));
		char * user = getenv("USER");
		char * loc = strstr(buf,user);
		if(loc != NULL){
			printf("%s:~%s> ",user,loc);
		}else{
			printf("%s:%s> ",user,buf);
		}
		fflush(stdout);
	}
}

extern "C" void disp (int sig){
	printf("\n");
	Command::_currentCommand.prompt();
}

extern "C" void zombie (int sig){
	int pid; 
	while((pid = waitpid(-1,NULL,WNOHANG)) > 0);
	int count;
	int found = 0;
	for (count = 0; count < bkgroundPIDsCounter; count++){
		if(bkgroundPIDs[count] == pid){
			for(int j = count + 1; j < bkgroundPIDsCounter; j++){
				bkgroundPIDs[j-1] = bkgroundPIDs[j];
			}
			bkgroundPIDsCounter--;
			found = 1;
			break;
		}
	}
	if (found){
		printf("%d exited. \n", pid);
		Command::_currentCommand.prompt();
	}
}

Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;

int yyparse(void);

main()
{
	bkgroundPIDs = (int *)malloc(sizeof(int) * 2048);
	bkgroundPIDsCounter = 0;
	
	struct sigaction signalINT;
	signalINT.sa_handler = disp;
	sigemptyset(&signalINT.sa_mask);
	signalINT.sa_flags = SA_RESTART;
	int error = sigaction(SIGINT, &signalINT, NULL);
	if(error){
		perror("sigINT");
		exit(-1);
	}
	struct sigaction signalZOM;
	signalZOM.sa_handler = zombie;
	sigemptyset(&signalZOM.sa_mask);
	signalZOM.sa_flags = SA_RESTART;
	int error2 = sigaction(SIGCHLD, &signalZOM,NULL);
	if(error2){
		perror("sigZOM");
		exit(-1);
	}

	Command::_currentCommand.prompt();
	yyparse();
}

