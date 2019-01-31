/*
  Implements a minimal shell.  The shell finds executables by
  searching the directories in the PATH environment variable.
  Specified executable are run in a child process.

  Alexander Hamme
  Assignment 2: BShell
  10/15/18
*/

#include "bshell.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <wait.h>

int parsePath(char *dirs[]);
char *lookupPath(char *fname, char **dir,int num);
int parseCmd(char *cmdLine, Command *cmd);

/*
  Read PATH environment var and create an array of dirs specified by PATH.

  PRE: dirs allocated to hold MAX_ARGS pointers.
  POST: dirs contains null-terminated list of directories.
  RETURN: number of directories.

  NOTE: Caller must free dirs[0] when no longer needed.
*/

int parsePath(char *dirs[]) {
  int i, numDirs;
  char *pathEnv;
  char *thePath;
  char *nextcharptr; /* point to next char in thePath */
  
  thePath = (char *) malloc(MAX_PATH_LEN+1);    // TODO dynamically allocate memory for this?

  for (i = 0; i < MAX_PATHS; i++) {
  	dirs[i] = NULL;
  }

  pathEnv = getenv("PATH");

  if (pathEnv == NULL) return 0; /* No path var. That's ok.*/

  /* for safety copy from pathEnv into thePath */
  strncpy(thePath,pathEnv, MAX_PATH_LEN);

#ifdef DEBUG
  printf("Path: %s\n",thePath);
#endif

  /* Now parse thePath */
  nextcharptr = thePath;

  /* 
     Find all substrings delimited by DELIM.  Make a dir element
     point to each substring.
     TODO: approx a dozen lines.
  */

  /* TODO:  split up thePath variable with strtok */

  char* token = strtok(thePath, DELIM);
  i = 0;
  numDirs = 0;
  while(token != NULL) {
    dirs[i++] = token;
    token = strtok(NULL, DELIM);
    numDirs++;
  }   


  /* Print all dirs */
#ifdef DEBUG
  for (i = 0; i < numDirs; i++) {
    printf("%s\n",dirs[i]);
  }
#endif
    
  return numDirs;
}


/*
  Search directories in dir to see if fname appears there.  This
  procedure is correct!

  PRE dir is valid array of directories
  PARAMS
   fname: file name
   dir: array of directories
   num: number of directories.  Must be >= 0.

  RETURNS full path to file name if found.  Otherwise, return NULL.

  NOTE: Caller must free returned pointer.
*/

char *lookupPath(char *fname, char **dir,int num) {
  char *fullName; // resultant name
  int maxlen; // max length copied or concatenated.
  int i;

  fullName = (char *) malloc(MAX_PATH_LEN);
  /* Check whether filename is an absolute path.*/
  if (fname[0] == '/') {
    strncpy(fullName,fname,MAX_PATH_LEN-1);
    if (access(fullName, F_OK) == 0) {
      return fullName;
    }
  }

  /* Look in directories of PATH.  Use access() to find file there. */
  else {
    for (i = 0; i < num; i++) {
      // create fullName
      maxlen = MAX_PATH_LEN - 1;
      strncpy(fullName,dir[i],maxlen);
      maxlen -= strlen(dir[i]);
      strncat(fullName,"/",maxlen);
      maxlen -= 1;
      strncat(fullName,fname,maxlen);
      // OK, file found; return its full name.
      if (access(fullName, F_OK) == 0) {
	return fullName;
      }
    }
  }
  free(fullName);
  return NULL;
}

/*
  Parse command line and fill the cmd structure.

  PRE 
   cmdLine contains valid string to parse.
   cmd points to valid struct.
  PST 
   cmd filled, null terminated.
  RETURNS arg count

  Note: caller must free cmd->argv[0..argc]

*/
int parseCmd(char *cmdLine, Command *cmd) {
  int argc = 0; // arg count
  char* token;
  int i = 0;

  token = strtok(cmdLine, SEP);
  while (token != NULL && argc < MAX_ARGS){    
    cmd->argv[argc] = strdup(token);
    token = strtok (NULL, SEP);
    argc++;
  }

  cmd->argv[argc] = NULL;  
  cmd->argc = argc;

#ifdef DEBUG
  printf("CMDS (%d): ", cmd->argc);
  for (i = 0; i < argc; i++)
    printf("CMDS: %s",cmd->argv[i]);
  printf("\n");
#endif
  
  return argc;
}


/* Prevent Ctrl-C from causing bshell to exit */
void ignoreSIGINT(int sig) {
	write(fileno(stdin), "\n", 2);
}


/*
  Run the shell
*/
int main(int argc, char *argv[]) {

	char *dirs[MAX_PATHS]; // list of dirs in environment
	int numDirs;
	char cmdline[LINE_LEN];

	int maxBackgroundJobs = 10;

    numDirs = parsePath(dirs);
    char args[MAX_ARGS][MAX_ARG_LEN];

	char *input;
	Command *command;

	// try to get username
	char *userName;
	if (((userName = getlogin()) == NULL) && ((userName = getenv("USER")) == NULL)) {
		userName = "user";  // default name if getting name was unsuccessful
	}

	// for listing jobs
	char *jobNames[maxBackgroundJobs];
	int jobIDs[maxBackgroundJobs];

	// initialize job name char arrays to NULL
	for (int i=0; i<maxBackgroundJobs; i++) {
		jobNames[i] = NULL;
		jobIDs[i] = 0;
	}

	int jobIndx = 0;

	// ignore Ctrl-C while in Terminal
	struct sigaction sigHandler;
	sigHandler.sa_handler = ignoreSIGINT;
	sigemptyset (&sigHandler.sa_mask);
	sigHandler.sa_flags = 0;
	sigaction (SIGINT, &sigHandler, NULL);

	while (1) {

		char* currDir = (char*) malloc(MAX_PATH_LEN + 1);
		getcwd(currDir, MAX_PATH_LEN);

		command = malloc(sizeof(Command) + 1);
	    input = (char *) malloc(MAX_ARGS * MAX_ARG_LEN + 1);

	    printf("%s@bshell:%s$ ", userName, currDir);

	    if (fgets(input, MAX_ARGS * MAX_ARG_LEN, stdin) == NULL || input[0] == '\n') {
			// if nothing was entered into shell, do nothing
		    continue;
	    }

	    int argc = parseCmd(input, command);

		/* "exit" makes the shell exit */
	    if (strncmp(command->argv[0], "exit", 4) == 0) {
	    	break;
	    }

		// if input contains "&" at end, and current number of backgrounded tasks is not at the maximum,
		// the new command will be run in the background
		int runInBackground = 0;
		if (command->argv[argc - 1][0] == '&') {
			if (jobIndx < maxBackgroundJobs) {
				runInBackground = 1;
				jobNames[jobIndx] = command->argv[0];
			} else {
				printf("Maximum number of background tasks already running, running job in foreground.\n");
			}
		}

		// parse input. compare against special commands first,
		// then try to 
	    for (int i=0; i<argc; i++) {

            if (strncmp(command->argv[i], "cd", 2) == 0 && i < argc-1) {

			    if (chdir(command->argv[i+1]) == 0) {       // Successfully changed directory
				    i++;         // this combined with main 'i++' will skip the next argument, the path passed to cd
				    continue;
			    }
            } else if (strncmp(command->argv[i], "jobs", 4) == 0) {  // list jobs
            	if (jobIndx == 0) {
            		printf("No current jobs in background\n");
            	} else {
		            for (int l = 0; l < jobIndx; l++) {
		            	if (jobNames[l] != NULL) {
				            printf("%d\t%s\n", jobIDs[l], jobNames[l]);
			            }
		            }
	            }
            	i++;
            	continue;
            } else if (strncmp(command->argv[i], "kill", 4) == 0) {

	            int sig = 15; // default signal
            	if (i < argc - 1) {  // if kill has at least one argument provided

            		// convert string termination signal to int
		            if (i < argc - 2 && command->argv[i + 1][0] == '-') {

			            char kSig[3];
			            // this works for both 1 and 2 digit signal number strings
			            sprintf(kSig, "%c%c", command->argv[i + 1][1], command->argv[i + 1][2]); // skip the hyphen
			            sig = (int) strtol(kSig, NULL, 0);

		            }

		            pid_t pidToKill = (pid_t) strtol(command->argv[i+2], NULL, 10);
            		kill(pidToKill, sig);

		            int jb=0;
            		for (; jb<jobIndx; jb++) {
			            if (jobIDs[jb] == pidToKill) {
				            jobNames[jb] = NULL;
				            jobIDs[jb] = 0;
			            }
		            }
            				// reposition job arrays to fill any null values between non-null values
            				for (int m=jb; m<jobIndx; m++) {    //
					            if (jobNames[m] == NULL) {      // replace this NULL value with the next non-NULL value, if there are any
						            for (int n = m + 1; n < jobIndx; n++) {
							            if (jobNames[n] == NULL) {
								            continue;
							            }
							            jobNames[m] = jobNames[n];
							            jobIDs[m] = jobIDs[n];
							            jobNames[n] = NULL;
							            jobIDs[n] = 0;
							            break;
						            }
					            }
            				}
            				// todo reposition arrays so empty spaces are all together at end
//            			}
//            		}
            		i += 2;
            	} else {
            		printf("Not enough arguments passed to kill command\n");
            	}
            	continue;
            }
			
			// if program reaches this point, the command did not match any of the above functions
			// therefore, it is either a reference to a native executable or an invalid command 

			// check if the first argument is a valid executable filename
			char * fullPath = lookupPath(command->argv[i], dirs, numDirs);

			if (fullPath != NULL) {  
				
				// parse the argument flags

				char *argsForCommand[MAX_ARGS];
				int flagIndx = 0;

				for (; flagIndx<MAX_ARGS; flagIndx++) {
					argsForCommand[flagIndx] = NULL;
				}

				flagIndx = 0;     // initialize the first argument as the program's own name, which is how argv is expected
				argsForCommand[flagIndx++] = fullPath;

				int j=++i;
				while(j<argc && command->argv[j] != NULL) {         // pass all

					// assume all subsequent non-valid program names are arguments or flags to be passed to previous command
					if (command->argv[j][0] != '&' && (command->argv[j][0] == '-' || lookupPath(command->argv[j], dirs, numDirs) == NULL)) {
						argsForCommand[flagIndx++] = command->argv[j++];
						i++;    // only increment i if the current argv value is a flag
					} else {
						break;
					}
				}

				pid_t pid = fork();

				if (pid == 0) {

					// run the executable, passing in the parsed argument flags
		            execv(fullPath, argsForCommand);

					for (int k=0; k<flagIndx; k++) {
						free(argsForCommand[k]);
					}

					_exit(0);       // exit child process after the program finishes

		        } else {

					if (runInBackground) {
						jobIDs[jobIndx++] = pid;
					} else {
						waitpid(pid, NULL, 0);
					}
		        }

	        } else {
				fprintf(stderr,"Command not found: %s\n", fullPath);
			}
	    }

	    free(currDir);
	    free(input);
	    free(command);
	  }

	  free(dirs[0]);

	return 0;
}
