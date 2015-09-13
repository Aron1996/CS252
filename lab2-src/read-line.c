/*
 * CS354: Operating Systems. 
 * Purdue University
 * Example that shows how to read one line with simple editing
 * using raw terminal.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

#define MAX_BUFFER_LINE 2048

// Buffer where line is stored
int line_length;;
char line_buffer[MAX_BUFFER_LINE];

// Simple history array
// This history does not change. 
// Yours have to be updated.
int history_index = 0;
char * history[50] = {0};
int history_length = sizeof(history)/sizeof(char *);

void read_line_print_usage()
{
  char * usage = "\n"
    " ctrl-?       Print usage\n"
    " Backspace    Deletes last character\n"
    " up arrow     See last command in the history\n";

  write(1, usage, strlen(usage));
}

/* 
 * Input a line with some basic editing.
 */
char * read_line() {

  struct termios original;
  tcgetattr(0,&original);
  // Set terminal in raw mode
  tty_raw_mode();

  line_length = 0;
  int history_location = 0;
  // Read one line until enter is typed
  while (1) {

    // Read one character in raw mode.
    char ch;
    read(0, &ch, 1);

    if (ch>=32 && ch != 127) {
      // It is a printable character. 

      // Do echo
      write(1,&ch,1);

      // If max number of character reached return.
      if (line_length==MAX_BUFFER_LINE-2) break; 

      // add char to buffer.
      line_buffer[line_length] = ch;
      line_length++;
    }
    else if (ch==10) {
      // <Enter> was typed. Return line
	  if(line_buffer[0] != '\0'){
	  	if(history_length == 50){
			history[history_length-1] = NULL;
	  	}
	  	history_length--;
	  	int i = history_length-1;
      	while(i > 0){
			history[i] = history[i-1];
			i--;
	  	}
		printf("\n%s\n", line_buffer);
		 history_length++;
		 history[0] = strdup(line_buffer);
      }
		
	  // Print newline
      write(1,&ch,1);

      break;
    }
    else if (ch == 31) {
      // ctrl-?
      read_line_print_usage();
      line_buffer[0]=0;
      break;
    }
    else if (ch == 8 || ch == 127) {
      // <backspace> was typed. Remove previous character read.

      // Go back one character
      ch = 8;
      write(1,&ch,1);

      // Write a space to erase the last character read
      ch = ' ';
      write(1,&ch,1);

      // Go back one character
      ch = 8;
      write(1,&ch,1);

      // Remove one character from buffer
      line_length--;
    }
	else if (ch == 1){
			
	}
	else if (ch == 4){
		
	}
	else if (ch == 5){
		
	}
    else if (ch==27) {
      // Escape sequence. Read two chars more
      //
      // HINT: Use the program "keyboard-example" to
      // see the ascii code for the different chars typed.
      //
      char ch1; 
      char ch2;
      read(0, &ch1, 1);
      read(0, &ch2, 1);
      if (ch1==91 && ch2==65) {
		// Up arrow. Print next line in history.

		// Erase old line
		// Print backspaces
		if(history[history_index] != NULL){		
			int i = 0;
			for (i =0; i < line_length; i++) {
			  ch = 8;
			  write(1,&ch,1);
			}
		
			// Print spaces on top
			for (i =0; i < line_length; i++) {
			  ch = ' ';
			  write(1,&ch,1);
			}
		
			// Print backspaces
			for (i =0; i < line_length; i++) {
			  ch = 8;
			  write(1,&ch,1);
			}	
		
			// Copy line from history
			strcpy(line_buffer, history[history_index]);
			line_length = strlen(line_buffer);
			history_index=(history_index+1)%history_length;
			history_location++;
			// echo line
			write(1, line_buffer, line_length);
		}      
     }
     else if(ch1 == 91 && ch2 == 66){
		/*// Down arrow
		if(history_location != 0){
		int i = 0;
		for (i =0; i < line_length; i++) {
		  ch = 8;
		  write(1,&ch,1);
		}
	
		// Print spaces on top
		for (i =0; i < line_length; i++) {
		  ch = ' ';
		  write(1,&ch,1);
		}
	
		// Print backspaces
		for (i =0; i < line_length; i++) {
		  ch = 8;
		  write(1,&ch,1);
		}
		// Copy line from history
		strcpy(line_buffer, history[history_index-1]);
		line_length = strlen(line_buffer);
		history_index=(history_index-1)%history_length;
		history_location--;
		// echo line
		write(1, line_buffer, line_length);
		}else {
			if(history_location != 0){
				int i = 0;
				for (i =0; i < line_length; i++) {
				  ch = 8;
				  write(1,&ch,1);
				}
			
				// Print spaces on top
				for (i =0; i < line_length; i++) {
				  ch = ' ';
				  write(1,&ch,1);
				}
			
				// Print backspaces
				for (i =0; i < line_length; i++) {
				  ch = 8;	
				  write(1,&ch,1);
				}
		}		
		}*/
	  }
	 else if(ch1 == 91 && ch2 == 67){
		// Right Arrow
		
		line_length++;
	  }
	 else if(ch1 == 91 && ch2 == 68){
		// Left Arrow
		line_length--;
	  }
	 else if (ch1 == 79 && ch2 == 70){
		// End Key
	 }
	 else if (ch1 == 79 && ch2 == 72){
		// Home Key
	 }
	 else {
		char ch3;
		read(0, &ch3, 1);
		if(ch1 == 91 && ch2 == 51 && ch3 == 126){
			// Delete
		}
	 }
    }

  }

  // Add eol and null char at the end of string
  line_buffer[line_length]=10;
  line_length++;
  line_buffer[line_length]=0;

  tcsetattr(0,TCSANOW,&original);

  return line_buffer;
}

