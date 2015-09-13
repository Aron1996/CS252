

const char * usage =
"                                                               \n"
"http-server:                                                   \n"
"                                                               \n"
"To use it in one window type:                                  \n"
"                                                               \n"
"   http-server [-f|-t|-p] <port>                               \n"
"                                                               \n"
"Different Concurrency options [-f|-f|-p] or empty              \n"
"	[Empty] - Runs a basic server with no concurrency       \n"
"	-f - Create a new Process for each request              \n"
"	-t Create a new Thread for each request                 \n"
"	-p Use a Pool of Threads                                \n"
"                                                               \n"
"Where 1024 < port < 65536.                                     \n"
"    if no Port is specified then a default port 4200 is used   \n"
;


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <dirent.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

int QueueLength = 5;

// Processes time request
void processRequest( int fd );
void processRequestThread(int socket);
void poolSlave(int socket);

pthread_mutex_t mutex;

extern "C" void killzombie( int sig ) {
	while(waitpid(-1,NULL,WNOHANG) > 0);
}

int
main( int argc, char ** argv )
{
  struct sigaction signalAction;
  signalAction.sa_handler = killzombie;
  sigemptyset(&signalAction.sa_mask);
  signalAction.sa_flags=SA_RESTART;
  int zom = sigaction(SIGCHLD,&signalAction,NULL);
  if (zom) { 
	perror("sigaction");
	exit(-1);
  }

  int port = 4200;
  int mode = 0;
  
  if(argc == 1){
	//default port and mode
  }else if(argc == 2){
	//default mode
	port = atoi(argv[1]);
  }else if (argc == 3){
	port = atoi(argv[2]);
	
	if(strcmp(argv[1], "-f") == 0){
		printf("Hello");
		mode = 1; // New Process
	}else if (strcmp(argv[1], "-t") == 0){
		mode = 2; // New Thread
	}else if(strcmp(argv[1], "-p") == 0){
		mode = 3; // Pool of Threads
	}else{
		fprintf(stderr, "%s", usage);
		exit(1);
	}
  }else {
	fprintf(stderr, "%s", usage);
	exit(1);
  }
   
 if(port < 1024 || port > 65536){
	fprintf(stderr, "%s", usage);
	exit(1);
 }
 

  // Set the IP address and port for this server
  struct sockaddr_in serverIPAddress; 
  memset( &serverIPAddress, 0, sizeof(serverIPAddress) );
  serverIPAddress.sin_family = AF_INET;
  serverIPAddress.sin_addr.s_addr = INADDR_ANY;
  serverIPAddress.sin_port = htons((u_short) port);
  
  // Allocate a socket
  int masterSocket =  socket(PF_INET, SOCK_STREAM, 0);
  if ( masterSocket < 0) {
    perror("socket");
    exit( -1 );
  }

  // Set socket options to reuse port. Otherwise we will
  // have to wait about 2 minutes before reusing the sae port number
  int optval = 1; 
  int err = setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR, 
		       (char *) &optval, sizeof( int ) );
   
  // Bind the socket to the IP address and port
  int error = bind( masterSocket,
		    (struct sockaddr *)&serverIPAddress,
		    sizeof(serverIPAddress) );
  if ( error ) {
    perror("bind");
    exit( -1 );
  }
    
  // Put socket in listening mode and set the 
  // size of the queue of unprocessed connections
  error = listen( masterSocket, QueueLength);
  if ( error ) {
    perror("listen");
    exit( -1 );
  }
  if(mode == 3){
	pthread_mutex_init(&mutex, NULL);
	
	pthread_t tid[QueueLength];
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

	for(int i = 0; i < QueueLength; i++){
		pthread_create(&tid[i], &attr, (void * (*)(void *))poolSlave, (void *)masterSocket);
	}
	pthread_join(tid[0], NULL);
  }else{
 	 while ( 1 ) {

 	   // Accept new TCP connection
 	   struct sockaddr_in clientIPAddress;
 	   int alen = sizeof( clientIPAddress );
	   int slaveSocket = accept( masterSocket,
				      (struct sockaddr *)&clientIPAddress,
				      (socklen_t*)&alen);
		
	   if(slaveSocket == -1 && errno == EINTR){
		continue;
	   }
	   if(mode == 0){
		  if ( slaveSocket < 0 ) {
	 	     perror( "accept" );
	 	     exit( -1 );
	 	   }
	 	// Read Request -> write Requested Document
		processRequest( slaveSocket );
			
	    	shutdown(slaveSocket,1);
	    	// Close TCP Connection
	    	close( slaveSocket );
	   }else if(mode == 1){
		pid_t slave = fork();
		if(slave == 0){
			processRequest(slaveSocket);
			shutdown(slaveSocket,1);
			//close(slaveSocket);
			exit(EXIT_SUCCESS);
		}
		
		//shutdown(slaveSocket,1);
		close(slaveSocket);
	   }else if(mode == 2){
		pthread_t tid;
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_create(&tid, &attr, (void *(*)(void(*)))processRequestThread, (void *)slaveSocket);
  	  }
  	}
  }
}

void poolSlave(int socket){
	while(1){
		struct sockaddr_in clientIPAddress;
		int alen = sizeof(clientIPAddress);
		
		pthread_mutex_lock(&mutex); 
		int slaveSocket = accept(socket,
			 (struct sockaddr *)&clientIPAddress,
			 (socklen_t*)&alen);
		pthread_mutex_unlock(&mutex); 

		processRequest(slaveSocket);
		shutdown(slaveSocket,1);
		close(slaveSocket);
	}
}

void processRequestThread(int socket){
	processRequest(socket);
	shutdown(socket,1);
	close(socket);
}


void
processRequest( int fd ) {
  const int len = 2048;
  char curr_string[len +1];

  char * docpath = (char*)calloc(len, sizeof(char));
  
  unsigned char newChar;
  unsigned char oldChar;
  
  int n;
  int length = 0;
  
  int seenGet = 0;
  int seenDocPath = 0;

  while(length < len && (n = read(fd, &newChar, sizeof(newChar)) > 0)){
	if(newChar == ' '){
		if(!seenGet){
			seenGet = 1;
		}else if (!seenDocPath){
			curr_string[length] = '\0';
			strcpy(docpath, curr_string);
			seenDocPath = 1;
		}
	}else if(newChar == '\n' && oldChar == '\r'){
		break;
	}else{
		oldChar = newChar;
		if(seenGet){
			length++;
			curr_string[length-1] = newChar;
		}
	}
  }
   
  while((n = read(fd ,&newChar, sizeof(newChar))) > 0){
  	if(newChar == '\r'){
		n = read(fd, &newChar, sizeof(newChar));
		if(newChar == '\n'){
			n = read(fd, &newChar, sizeof(newChar));
			if(newChar == '\r'){
				n = read(fd, &newChar, sizeof(newChar));	
				if(newChar == 10){
					break;
				}	
			}
		}
	}	
  }
  char *cwd = (char*)calloc(256, sizeof(char));
  cwd = getcwd(cwd, 256*sizeof(char));
  int cgi = 0;
  int filePathLength = strlen(cwd) + strlen("/http-root-dir");
  if(docpath[0] ==  '/' && (docpath[1] != '\0')){
	int i = 0;
 	char *start = (char*)calloc(2048,sizeof(char));
	
	while(docpath[i+1] != '\0'){
		if(docpath[i+1] == '/'){
			break;
		}
		if(docpath[i+1] == '?'){
			break;
		}
		start[i] = docpath[i+1];
		
		i++;
	}
	
	if(strcmp(start,"icons") == 0 || strcmp(start,"htdocs") == 0){
		strcat(cwd,"/http-root-dir");
		strcat(cwd,docpath);
	}else if(strcmp(start, "cgi-bin") == 0){
		cgi = 1;
		strcat(cwd,"/http-root-dir");
		strcat(cwd,docpath);
	}
	else{	
		strcat(cwd,"/http-root-dir/htdocs");
		strcat(cwd,docpath);
	}
  }
  else{	
  	strcat(cwd,"/http-root-dir/htdocs/index.html");
  }

	
  //setenv("QUERY_STRING", " ", 1);
  setenv("REQUEST_METHOD", "GET", 1);

  char * envVar = strstr(cwd,"?");
  if(envVar != NULL){
	envVar++;
	char * tmp;
	
	tmp = strdup(envVar);
	envVar = tmp;
	cwd[strlen(cwd)-(strlen(envVar)+1)] = '\0';
	
	setenv("QUERY_STRING", strdup(envVar), 1);
	setenv("REQUEST_METHOD", "GET", 1);
 
	if(strstr(envVar, "=")){
		envVar[strlen(envVar)] = '&';
		envVar[strlen(envVar)+1] = '\0'; 
		char * varAndVal;
		varAndVal  = strtok(envVar, "&");
		
		while(varAndVal != NULL){
			char * val = varAndVal;
			int counter = 0;
			while(*val != '='){
				val++;
				counter++;
			}	
			char * t;
			t = strdup(val+1);
			varAndVal[counter] = '\0';
			setenv(varAndVal, t,1);
			varAndVal = strtok(NULL, "&");
		}
	}
	
  }else{
  	setenv("QUERY_STRING", NULL, 1);
  }
  
  if (strstr(cwd, "..") != 0){
  	char * realPath = (char*)calloc(len+1, sizeof(char));
	realPath = realpath(cwd,realPath);
	if(strlen(realPath) > filePathLength){
		cwd = realPath;
	}
	
  }
  char * contentType;

  if(strstr(cwd,".html") || strstr(cwd,".html/")){
	contentType = strdup("text/html");
  }else if(strstr(cwd,".gif") || strstr(cwd,".gif/")){
	contentType = strdup("image/gif");
  }else{
	contentType = strdup("text/plain");
  }
	
  	
  FILE * file;
  DIR * dir;
  const char * clrf = "\r\n";
  const char * serverType = "CS 252 Lab 4";
  const char * protocol = "HTTP/1.1";
  if((file = fopen(cwd,"r")) != NULL){ // No Error 
	
	write(fd, protocol, strlen(protocol));
	write(fd, " ", 1);
	write(fd, "200", 3);
	write(fd, " ", 1);
	write(fd, "Document", 8);
	write(fd, " ", 1);
	write(fd, "follows", 7);
	write(fd, clrf, 2);
	write(fd, "Server:", 7);
	write(fd, " ", 1);
	write(fd, serverType, strlen(serverType));
	write(fd, clrf, 2);
	
	if(cgi){
		fclose(file);
		FILE *fp = popen(cwd, "r");
		
		char c;
		int fileNo = fileno(fp);
		while((n = read(fileNo, &c, sizeof(c))) > 0){
			write(fd, &c, sizeof(c));
		}
		fclose(fp);
	}else{
		write(fd, "Content-type:", 13);
		write(fd, " ", 1);
		write(fd, contentType, strlen(contentType));
		write(fd, clrf, 2);
		write(fd, clrf, 2);
		char c;
		int fileNo = fileno(file);
		while((n = read(fileNo, &c, sizeof(c)))> 0){
			write(fd, &c, sizeof(c));
		}
		fclose(file);
	}
	
  }else{ // Error	
	contentType = strdup("text/plain");
	const char *notFound = "File not Found";
	write(fd, protocol, strlen(protocol));
	write(fd, " ", 1);
	write(fd, "404", 3);
	write(fd, "File", 4);
	write(fd, "Not", 3);
	write(fd, "Found", 5);
	write(fd, clrf, 2);
	write(fd, "Server:", 7);
	write(fd, " ", 1);
	write(fd, serverType, strlen(serverType));
	write(fd, clrf, 2);
	write(fd, "Content-type:", 13);
	write(fd, " ", 1);
	write(fd, contentType, strlen(contentType));
	write(fd, clrf, 2);
	write(fd, clrf, 2);
	write(fd, notFound, strlen(notFound)); 
  }
  
}
