/*
 * Mechanics of Programming Project 3
 * File: mysh.c
 * @author: Mike Cao 
 * @date: 4/11/19
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>	// fork
#include <sys/wait.h>	// wait
#include <sys/types.h>  
#define MAX_STRING 1024	// max string length 

// recurse guard
int guard = 0;
// sequence number
int sequence = 1;
// verbose on/off 
int verboseFlag = 0;
// max number of commands
int MAX_HISTORY = 10;
// function declaration
char *s_strtok(char * command, const char *delim1, const char *delim2, char **ptr);

/*
 * HELPER FUNCTION
 * strdup function copies a string
 * not standard C so a custom function is included
 * @param str the string to copy
 */
char * strdup(const char * str) {
	char * ret = (char*)malloc(strlen(str) + 1);
	if(ret)
		strcpy(ret, str);
	return ret;
}

/*
 * INTERNAL COMMAND FUNCTION
 * prints the help menu
 */
int mysh_help(int argc, char * argv[]) {
	// unused so cast them to void
	(void)argc;
	(void)argv;
	// print the internal ocmmands
        printf("Internal User Commands for mysh:\n");
        printf("!N\t\t# re-execute the Nth command in the history list.\n");
        printf("help\t\t# print all internal commands.\n");
        printf("history\t\t# print a list of commands executed so far.\n");
        printf("quit\t\t# free all dynamic memory and exit the program.\n");
        printf("verbose on/off\t# turn the shell verbose on or off.\n");
	return 0;
}

/*
 * INTERNAL COMMAND FUNCTION
 * prints the history the most recent commands up to the MAX_HISTORY back
 * @param argc the sequence number
 * @param argv the history array
 */
int mysh_history(int argc, char * argv[]) {
	int adjustedSequence;
	int sequence = argc;
	// adjust the sequence number to the index of the history array
	if(sequence > MAX_HISTORY) {
                adjustedSequence = sequence - MAX_HISTORY;
        } else {
                adjustedSequence = 1;
        }
	// print the items in the history array
        for(int i = 0; i < MAX_HISTORY; i++) {
                if(argv[i] != NULL)
                        printf("\t%d: %s\n", adjustedSequence++, argv[i]);
        }	
	return 0;
}

/*
 * INTERNAL COMMAND FUNCTION
 * mysh_bang looks into the command with the same sequence number as the provided
 * argument and re-executes that command
 * @param argc the bang number or sequence number
 * @param argv the array history
 */
int mysh_bang(int argc, char * argv[]) {
	int bangNum = argc;
	// check if the bang number is valid 
	// check if the bang num is greater than the previous sequence number
	if(bangNum >= (sequence - 1)) {
                fprintf(stderr,
                        "!%d cannot be completed since it doesn't exist.\n",
                        bangNum);
                return 1;
	// check if the bang num has been forgotten by the shell
        } else if(((sequence - 1) - bangNum > MAX_HISTORY)) {
                fprintf(stderr,
                        "!%d is lost. Only !%d-!%d can be accessed.\n",
                        bangNum, sequence - MAX_HISTORY, sequence - 1);
                return 1;
        }
	// adjust the sequence number to the index of the history array
        int adjustedSequence;
        if(bangNum > MAX_HISTORY) {
                adjustedSequence = MAX_HISTORY - ((sequence - 1) - bangNum);
        } else {
                adjustedSequence = bangNum - 1;
        }
	// get the appropriate command in the history array
        char * tok = NULL;
        char * temp = (char*)malloc(strlen(argv[adjustedSequence]) + 1);
        strcpy(temp, argv[adjustedSequence]);
	char * cpy = strdup(temp);
	// if verbose is on print the command number and command
	if(verboseFlag)
		printf("\n\tcommand %d: %s\n\n", argc, temp);
	// tokenize
        tok = strtok(temp, " \n");
        // check what command it is but don't store it
	// if the command is help
        if(!strcmp(tok, "help")) {
		// help function
                mysh_help(0, NULL);
	// if the command is history
        } else if(!strcmp(tok, "history")) {
		// print the history
                mysh_history(0, argv);
	// if the command is a bang command
        } else if(tok[0] == '!') {
		// get the number part only after checking if there is a
		// ! in the zero index of the command
                char temp2[MAX_STRING];
                memcpy(temp2, &tok[1], strlen(tok));
                temp[strlen(tok)] = '\0';
                int bangNum = (int)strtol(temp2, NULL, 10);
		// check if it is recursing
		if(guard++ < 10) {
                	mysh_bang(bangNum, argv);
		// if infinite recursing, stop it
		} else {
			guard = 0;
			fprintf(stderr, "infinite recursing command\n");
		}
	// if the command is verbose
        } else if(!strcmp(tok, "verbose")) {
		// tokenize
		tok = strtok(NULL, " \"\'\n");
		if(tok == NULL) {
			fprintf(stderr, "usage: verbose on | off\n");
		}
		// check the argument and turn it off or on appropriately
		if(!strcmp(tok, "on")) {
			verboseFlag = 1;
		} else if(!strcmp(tok, "off")) {
			verboseFlag = 0;
		} else {
			// print usage statement since argument was incorrect
			fprintf(stderr, "usage: verbose on | off\n");
		}
	} else {
		// external command
		int status;
		// fork to create child process for execvp
		pid_t id = fork();
		// check if the return value is an error
		if(id < 0) {
			perror("fork");
			return EXIT_FAILURE;
		// execute only if child process
		} else if(id == 0) {
			// temporary argv
			char * targv[MAX_STRING];
			// token for strtok
			char * param;
			// number of parameters
			size_t index = 0;
			// get the first word in the input
			// which is the command
			param = strtok(cpy, " \n");
			// add each argument and add it to argv
			while(param != NULL &&
					index <
					(MAX_STRING - 1)) {
				param = strtok(NULL, " \n");
				if(param)
					targv[index++] =
						strdup(param);
			};
			// create the final argv with 2 extra
			// spots for the command and the NULL
			// pointer
			char * fargv[index + 2];
			// set it all the NULL for debugging
			// purposes
			for(size_t i = 0; i < index + 2; i++)
				fargv[i] = NULL;
			// create a string and assign it to
			// the first spot in fargv
			fargv[0] = strdup(tok);
			// add each argument to fargv after
			// the command name
			for(size_t i = 0; i < index; i++) 
				fargv[i + 1] = strdup(targv[i]);		
			// assign a NULL pointer
			fargv[index + 1] = NULL;
			// execute external command
			execvp(tok, fargv);
			// executes only if execvp fails
			perror("execvp");
			_exit(EXIT_FAILURE);
		}
		// parent process waits for the child process
		id = wait(&status);
		if(id < 0) {
			// error
			perror("wait");
		} else {
			// for debugging and checking the
			// status of the child process
			#ifdef DEBUG
			printf("WIFEXITED: %d\n",
				WIFEXITED(status));
			printf("child %d, status %d\n",
				id, WEXITSTATUS(status));
			#endif
		}

	}
	free(cpy);
        free(temp);
	return 0;
}

/*
 * INTERNAL COMMAND FUNCTION
 * verbose prints the tokens of the command
 * @param argv history array
 */
int mysh_verbose(int argc, char * argv[]) {
	// argc is unused
	(void)argc;
	// get the appropriate command which is the previous command
	char * command = NULL;
	if(argv[sequence - 2] && sequence - 2 < MAX_HISTORY) {
		command = strdup(argv[sequence - 2]);
	} else {
		command = (char*)strdup(argv[MAX_HISTORY - 1]);
	}
	// print the diagnostic statements
	printf("\tcommand: %s\n", command);
	printf("\n\tinput command tokens:\n");
	int tokNum = 0;
	char * token = NULL;
	do {
		token = s_strtok(command, " \n", "\"\'", &command);
		printf("\t%d: %s\n", tokNum++, token);
		//token = strtok(NULL, " \n");
	} while(token != NULL);
	return 0;	
}

/*
 * HELPER FUNCTION
 * storeCommand stores the command in the array
 * @param argv the array history
 * @param command the command to store
 */
void storeCommand(char * argv[], char * command) {
        size_t added = 0;
	// look for an expty index in the array
        for(int i = 0; i < MAX_HISTORY; i++) {
                if(argv[i] == NULL) {
			// if empty, store the command there
                        argv[i] = (char*)malloc(MAX_STRING);
                        strcpy(argv[i], command);
                        added = 1;
                        break;
                }
        }
        // check if the command was not stored stored 
        if(!added) {
		// free the first index since it will be forgotten
                if(argv[0])
                        free(argv[0]);
		// move the indexes back one
                for(int i = 0; i < MAX_HISTORY - 1; i++) {;
                        strcpy(argv[i], argv[i + 1]);
                }
		// free the last index since it will be replaced
		if(argv[MAX_HISTORY - 1])
                	free(argv[MAX_HISTORY - 1]);
                // replace the last index with the command
		argv[MAX_HISTORY - 1] = strdup(command);
        }
}

/*
 * HELPER FUNCTION
 * freeHistory frees the dynamically allocated character pointers inside the
 * array when the program quits
 * @param argv the history
 */
void freeHistory(char * argv[]) {
	// free evertyhing in history array
        for(int i = 0; i < MAX_HISTORY; i++) {
                if(argv[i])
                        free(argv[i]);
        }
}

/*
 * s_strtok parses the command into tokens with quotes first then space
 * @param command the command
 * @param delim1 second priority delimiter
 * @param delim2 first priority dilimiter
 * @param ptr address of the original command
 */
char *s_strtok(char * command, const char *delim1, const char *delim2, char **ptr) {
	char *end;
    	size_t s1;
    	size_t s2;
	// true/false for whether or not the first priority delimiter was found
	int findDelim2 = 0;
	// check if there is a command
	if (command == NULL)
		command = *ptr;
	// check if there is a null pointer
	if (*command == '\0') {
		*ptr = command;
		return NULL;
	}
	// get the length of the substring that has the delimiter
	s1 = strspn(command, delim1);
	s2 = strspn(command, delim2);
	// if the first priority delimiter is encountered first...
	if(s2 > s1) {
		// go to the delimiter
		command += s2;
		findDelim2  = 1;
	} else {
		// do the same 
		command += s1;
	}
	// if the result of the prevous operations give us a null pointer
	if (*command == '\0') {
		// return the resulting tokenized substring
		*ptr = command;
		return NULL;
	}

	// find the end of the token
	if(findDelim2) {
		// go to the end of the second occurence
		end = command + strcspn(command, delim2);
	} else {
		// do the same
		end = command + strcspn(command, delim1);
	}
	// if the end is a null pointer
	if (*end == '\0') {
		// return the command
		*ptr = end;
		return command;
	}

	// Terminate the token and make ptr point past it
	*end = '\0';
	*ptr = end + 1;
	return command;
}

/*
 * main function parses the input through stdin and executes functions or 
 * commands based on its interpretation of the input
 * @param argc number of command line arguments
 * @param argv array of char* command line arguments
 * return EXIT_SUCCESS if normal exit
 * return EXIT_FAILURE if something goes wrong
 */
int main(int argc, char * argv[]) {
	// checks if there are more than three command line arguments
	if(argc > 3) {
		// print usage statement
		fprintf(stderr, "usage: mysh [-v] [-h pos_num]\n");
		return EXIT_FAILURE;
	// check the second argument
	} else if(argc == 2) {
		// if the second argument is the verbose flag
		if(!strcmp(argv[1], "-v")) {
			// turn on verbose mode
			verboseFlag = 1;
		} else {
			// pointer where the num number parts will be
			// stored in
			char * str = NULL;
			// convert to int
			int tempHistoryNum = (int)strtol(argv[1], &str, 10);
			// if there were any non integers, then then exit
			// because the optional argument is invalid
		 	if(strcmp(str, "")) {
				fprintf(stderr, 
					"usage: mysh [-v] [-h pos_num]\n");
				exit(EXIT_FAILURE);
			}
			// if valid, set MAX_HISTORY to the provided argument
			MAX_HISTORY = tempHistoryNum;
		}
	// check the second and third argument
	} else if(argc == 3) {
		// check for the verbose flag in the first argument
		if(!strcmp(argv[1], "-v")) {
			verboseFlag = 1;
		} else {
			fprintf(stderr, "usage: mysh [-v] [-h pos_num]\n");
                        exit(EXIT_FAILURE);
		}
		// check for the second argument and see if it is an numerical
		// value
                char * str = NULL;
                int tempHistoryNum = (int)strtol(argv[2], &str, 10);
                if(strcmp(str, "")) {
                	fprintf(stderr, "usage: mysh [-v] [-h pos_num]\n");
                        exit(EXIT_FAILURE);
		}
		MAX_HISTORY = tempHistoryNum;
	}						
	// the history of commands
	char * arrayOfCommands[MAX_HISTORY];
	// initialize everything in the aray
	for(int i = 0; i < MAX_HISTORY; i++) 
		arrayOfCommands[i] = NULL;	
	// infinite loop
	while(1) {
		// reset guard
		guard = 0;
		// hold the inputs
		char input[MAX_STRING];
                char tInput[MAX_STRING];
		// prompt
		printf("mysh[%d]> ", sequence++);
		fflush(stdout);
		// receive the response
		if(fgets(input, MAX_STRING, stdin) != NULL && 
				strcmp(input, "\n")) {
			// get the first token to see what command it is
			char * cpy = strdup(input);
			strcpy(tInput, strtok(input, " \n"));
			// check if the command is a bang command by checking
			// if the first character is a '!'
			if(input[0] == '!') {	
				// get the number part of the bang command
                                char temp[MAX_STRING];
				// checks if there is anything other than 
				// numbers in the bang command aside from !
				char * checker = NULL;
				// store the command
				cpy = strtok(cpy, "\n");
				storeCommand(arrayOfCommands, cpy);
				free(cpy);
				// if verbose is on, print the diagnostics
				if(verboseFlag)
					mysh_verbose(0, arrayOfCommands);
				// get the integer number of the command
                                memcpy(temp, &tInput[1], strlen(tInput));
                                temp[strlen(tInput)] = '\0';
				// convert it to int to pass to bang function
                                int bangNum = (int)strtol(temp, &checker, 10);
				// checks if there were anything other than
				// numbers
				if(strcmp(checker, "")) {
					// if so, error
					fprintf(stderr, "usage: !N\n");
					// since the bang command failed
					// the command status will be non zero
					if(verboseFlag)
						printf("command status: 2\n");
					break;
				}
				// bang function
                                mysh_bang(bangNum, arrayOfCommands);	
				// if verbose print command status which is 0
				// because it most probably succeeded
				if(verboseFlag)
					printf("command status: 0\n");		
			// check if the command is history
			} else if (!strcmp(tInput, "history")) {
				// store the command
				cpy = strtok(cpy, "\n");
				storeCommand(arrayOfCommands, cpy);
				// if verbose is on, print the diagnostics
				if(verboseFlag) {
					mysh_verbose(0, arrayOfCommands);
					printf("\n");
				}
				// print the history
				mysh_history(0, arrayOfCommands);
				// command status is 0 since the command 
				// succeeded
				printf("command status: 0\n");
			// check if the command is help
			} else if(!strcmp(tInput, "help")) {
				// store the command
				cpy = strtok(cpy, "\n");
				storeCommand(arrayOfCommands, cpy);
				// if verbose is on, print the diagnostics
				if(verboseFlag)
					mysh_verbose(0, arrayOfCommands);
				// print the help menu
				mysh_help(0, NULL);
				// the command status is 0 since it is almost 
				// impossible for help to fail
				printf("command status: 0\n");
			// check if the command is quit
			} else if(!strcmp(tInput, "quit")) {
				// don't need to store command and verbose 
				// because quitting will exit 
				// free dynamic memory
				freeHistory(arrayOfCommands);
				return EXIT_SUCCESS;
			// check if verbose command
			} else if(!strcmp(tInput, "verbose")) {
				// get the argument for the input
				char * status = strtok(NULL, "\"\'\n");
				// store the command
				cpy = strtok(cpy, "\n");
                                storeCommand(arrayOfCommands, cpy);
				if(status) {
					// check the argument for verbose
					// and change the flag to true or false
					// depending on the argument
					if(!strcmp(status, "on")) {
						verboseFlag = 1;
					} else if(!strcmp(status, "off")) {
						verboseFlag = 0;
					} else {
						// error
						fprintf(stderr, "usage: verbose on | off\n");
					}
				} else {
					// error
					fprintf(stderr,
						"usage: verbose on | off\n");
				} 
			} else {
				// store the command
				cpy = strtok(cpy, "\n");
				storeCommand(arrayOfCommands, cpy);
				// external command
				int status;
				int parentID = 0;
				// fork to create child process for execvp
				pid_t id = fork();
				if(id != 0 && verboseFlag) {
					parentID = (int)id;
					mysh_verbose(0, arrayOfCommands);
                                        printf("\twait for pid %d: %s\n", 
						parentID, tInput);
                                        printf("\texecvp: %s\n", tInput);
				}
				// check if the return value is an error
			       	if(id < 0) {
					perror("fork");
					exit(EXIT_FAILURE);
				// execute only if child process
				} else if(id == 0) {
					// temporary argv 
					char * argv[MAX_STRING];
					// token for strtok
					char * param;
					// number of parameters
					size_t index = 0;
					// get the first word in the input
					// which is the command
					//param = strtok(cpy, " \n");
					// add each argument and add it to argv
					do {
						param = s_strtok
							(cpy, " \n", "\"\'", 
							&cpy);
						if(param)
							argv[index++] = 
								strdup(param);
					}  while(param != NULL &&
                                                        index <
                                                        (MAX_STRING - 1));
					// create the final argv with 2 extra
					// spots for the command and the NULL
					// pointer
					char * fargv[index + 2];
					// set it all the NULL for debugging
					// purposes
					for(size_t i = 0; i < index + 2; i++)
						fargv[i] = NULL;
					// create a string and assign it to
					// the first spot in fargv
					//fargv[0] = strdup(tInput);
					// add each argument to fargv after
					// the command name
					for(size_t i = 0; i < index; i++) 
						fargv[i] = strdup(argv[i]);
					// assign a NULL pointer
					fargv[index + 1] = NULL;
					// execute external command
					execvp(tInput, fargv);
					// executes only if execvp fails
					perror("execvp");
					_exit(EXIT_FAILURE);
				}
				// parent process waits for the child process
				id = wait(&status);
				if(verboseFlag) {
					printf("command status: %d\n",
						WEXITSTATUS(status));
				}
				if(id < 0) {
					// error
					perror("wait");
				} else {
					// for debugging and checking the
					// status of the child process
					#ifdef DEBUG
					printf("WIFEXITED: %d\n",
						WIFEXITED(status));
					printf("child %d, status %d\n", 
						id, WEXITSTATUS(status));
					#endif
				}
			}
		} else {
			// input wasn't received
			fprintf(stderr, "Input was empty.\n");
		}
	}
	return 0;
}
