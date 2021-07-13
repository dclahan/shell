#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>


int main()
{
	//	initialize outside of loop
	setvbuf( stdout, NULL, _IONBF, 0 );
	int run = 1;
	char *my_path = calloc(1024,sizeof(char));
	if (getenv("MYPATH") == NULL)
		strcpy(my_path,"/bin:.");
	else
		strcpy(my_path,getenv("MYPATH"));
	//  Loop starts here
	while (run > 0){
		//WAITPID NONBLOCKING CALL
		int stutas;
	    pid_t child_pid = 1;
	    while (child_pid > 0){
		    child_pid = waitpid( -1, &stutas, WNOHANG);   /*NON-BLOCKING, WNOHANG instead of 0*/	
		    if (child_pid > 0){	 //background process over
		    	if ( WIFSIGNALED( stutas ) )  /* child process was terminated   */
			    {                             /*  by a signal (e.g., seg fault) */
			      fprintf(stderr, "<background process terminated abnormally>\n" );
			      // return EXIT_FAILURE;
			    }
			    else if ( WIFEXITED( stutas ) )
			    {
			      char exit_status = WEXITSTATUS( stutas );
			      printf( "<background process terminated with exit status %d>\n", exit_status );
			    }
		    }
		}
		//  initialize inside of loop
		char *buffer = calloc(1024,sizeof(char));		//buffer for words
		printf("$ ");
		fgets(buffer, 1024, stdin);
		if (strlen(buffer) == 1 || strlen(buffer) == 0){free(buffer); continue;}
		if (strlen(buffer)>0 && *(buffer + strlen(buffer)-1) == '\n'){*(buffer + strlen(buffer)-1) = '\0';}
		const unsigned int buf_len = strlen(buffer);
		//parse buffer spacewise

		//with token
		char *tok_buffer = calloc(1024,sizeof(char));	//copy of buffer
		strcpy(tok_buffer, buffer);
		char *token;
		const char *s = " ";
		token = strtok(tok_buffer, s);
		char *first_tok = calloc(256,sizeof(char));
		strcpy(first_tok, token);
		
		char **commandline_parsed = calloc(1024,sizeof(char*));
		*(commandline_parsed) = calloc(256,sizeof(char));
		int num_args = 1;
		token = strtok(NULL, s); 	//if you want the first token to NOT be in the array of args (turns out we do but will put in l8r bc lay-z)
		while (token != NULL){ 		//populate args 
			if (strcmp(token,"&") == 0){
				break;
			}
			*(commandline_parsed+num_args) = calloc(256,sizeof(char));
			sprintf(*(commandline_parsed+num_args),"%s",token);
			num_args++;
			token = strtok(NULL, s);
		}
		token = first_tok;
		free(tok_buffer);

		//test for exit or cd
		if (strcmp(token,"exit") == 0 && *(token)!='o' && *(token+2)!='s') {
			printf("bye\n");
			run = 0;
			free(buffer);
			free(first_tok);
			for (int i = 0; i < num_args; ++i){
				free(*(commandline_parsed+i));
			}
			free(commandline_parsed);
			continue;
		} else if (strcmp(token,"cd") == 0 && *(token)!='m' && *(token+1)!='n') {
			//cdeeznuts
			int cdrom;
			if (num_args == 1) {
				//no arguments, use base dir
				cdrom = chdir(getenv("HOME"));
				if (cdrom<0){printf("chdir didnt work\n");}
			} else {
				//use arguments
				if (chdir(*(commandline_parsed+1))<0) {printf("chdir(\"%s\") didnt work\n",*(commandline_parsed+1));}
			}
		} 
		//fork calling action
		 else {  //FOREGROUND PROCESSING
				 //otherwise find the path and do the thing with the things
				//locate command excecutable

			char *my_path_tokenizeable = calloc(1024,sizeof(char));	//copy of my_path to tokenize
			strcpy(my_path_tokenizeable,my_path);
			// now, tokenize my_path after each ':', concat <token> and run lstat 
			struct stat stats;
			int command_found = 0;
			char *path_tok;
			const char *col = ":";
			char *copied = calloc(256,sizeof(char));	//copy of path_tok
			path_tok = strtok(my_path_tokenizeable,col);
			strcpy(copied, path_tok);
			strcat(copied,"/");
			strcat(copied,token);						//copied will be used in the execv function to find the command (duh)
			int r = lstat(copied,&stats);				// once copied is found, it must be preserved for later use
		    path_tok = strtok(NULL, col);
			if (r < 0){
				while( path_tok != NULL ) {
					memset(copied, '\0',256); //zero out bad path
					strcpy(copied, path_tok);
					strcat(copied,"/");
					strcat(copied,token);
					int r = lstat(copied,&stats);
					if (r == 0){
						command_found = 1;
						break;
					}
				    path_tok = strtok(NULL, col);
				}
			} else {
				command_found = 1;
			}
			free(my_path_tokenizeable);
			if (command_found == 0){
				fprintf(stderr,"%s: command not found\n", token);
				free(copied);
				free(buffer);
				free(first_tok);
				for (int i = 0; i < num_args; ++i){
					free(*(commandline_parsed+i));
				}
				free(commandline_parsed);
				continue;
			}
				//otherwise
				// fork() and run
			strcpy(*(commandline_parsed),token);
			pid_t p;
			/* create a new (child) process */
			p = fork();
			if ( p == -1 ) {
				perror( "fork() failed" );
				return EXIT_FAILURE;
			}


			if ( p > 0 && *(buffer+buf_len-1)!='&')   /* PARENT process executes here... */
			{
			 	int status;
			    waitpid( p, &status, 0);   /* BLOCKING */ /*FOR NON-BLOCKING, use WNOHANG instead of 0*/
			    free(copied);
			    //exit status check
			} else if (p > 0 && *(buffer+buf_len-1)=='&') {
				printf("<running background process \"%s\">\n", token);
				free(copied);
			} else if ( p == 0 )   /* CHILD process executes here... */
			{
				execv(copied, commandline_parsed);
				perror( "execv() failed" );
				return EXIT_FAILURE;
			}
		}
		// free(token);
		free(buffer);
		free(first_tok);
		for (int i = 0; i < num_args; ++i){
			free(*(commandline_parsed+i));
		}
		free(commandline_parsed);
	}
	free(my_path);
	return 0;
}