
/*
 *
 * CS-252
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

%{

#include <string.h>
#include "y.tab.h"

#include <unistd.h>
/*
////////////  Start added code ///////////

extern "C" char * read_line();

int mygetc(FILE * f) {
    static char *p;
    char ch;

      	if (!isatty(0)) {
        	// stdin is not a tty. Call real getc
	      	return getc(f);
	}

	// stdin is a tty. Call our read_line.

	if (p==NULL || *p == 0) {
	    	char * s = read_line();
	    	p = s;
      	}

        ch = *p;
	p++;

	return ch;
    }

#undef getc
#define getc(f) mygetc(f)

////////// End added code ///////////
*/

static  void yyunput (int c,char *buf_ptr  );

void myunputc(int c) {
    unput(c);
}

%}

%%




\n 	{
		return NEWLINE;
	}

[ \t] 	{
		/* Discard spaces and tabs */
     	}	

">" 	{
		return GREAT;
	}

"|"	{
		return PIPE;
	}

"<"	{
		return LESS;
	}

">&"	{
		return GREATAMPERSAND;
	}

">>"	{
		return GREATGREAT;
	}

">>&"	{
		return GREATGREATAMPERSAND;
	}

"&"	{
		return AMPERSAND;
	}


\`[^\n\']*\` 	{
		yylval.string_val = yytext;
		return SUBSHELLCOMMAND;
	}


\"[^\n\"]*\"	{
		yytext = strdup(yytext+1);
		int len = strlen(yytext);
		if(yytext[len-1] == '"'){
			yytext[len-1] = '\0';
		}
		yylval.string_val = yytext;
		return WORD;
	}

[^ \t\n<>&'\|]*\\[^ \t\n]* 	{
		
		char * ret = (char *)malloc(sizeof(char) * strlen(yytext));
		char * back = ret;
		while(*yytext != '\0'){
			if(*yytext == '\\'){
				if(*(yytext+1) == '$' && *(yytext+2) == '{'){			
					*ret = *yytext;
					ret++;
					yytext++;
				}else{
					yytext++;
					*ret = *yytext;
					ret++;
					yytext++;
				}
			}else{
				*ret = *yytext;
				ret++;
				yytext++;
			}
		}
		yylval.string_val = back;		
		return WORD;
	}		

[^ \t\n|<>&][^ \t\n|<>&]*		{
		/* Assume that file names have only alpha chars */
		yylval.string_val = strdup(yytext);
		return WORD;
	}

