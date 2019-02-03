/*
  Implements a minimal shell.  The shell finds executables by
  searching the directories in the PATH environment variable.
  Specified executable are run in a child process.

  Alexander Hamme, Keith O'Hara
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
#include <pthread.h>
#include <errno.h>

int parsePath(char *dirs[]);

char *lookupPath(char *fname, char **dir, int num);

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

	thePath = (char *) malloc(MAX_PATH_LEN + 1);    // TODO dynamically allocate memory for this?

	for (i = 0; i < MAX_PATHS; i++) {
		dirs[i] = NULL;
	}

	pathEnv = getenv("PATH");

	if (pathEnv == NULL) return 0; /* No path var. That's ok.*/

	/* for safety copy from pathEnv into thePath */
	strncpy(thePath, pathEnv, MAX_PATH_LEN);

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

	char *token = strtok(thePath, DELIM);
	i = 0;
	numDirs = 0;
	while (token != NULL) {
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

char *lookupPath(char *fname, char **dir, int num) {
	char *fullName; // resultant name
	size_t maxlen; // max length copied or concatenated.
	int i;

	fullName = (char *) malloc(MAX_PATH_LEN);
	/* Check whether filename is an absolute path.*/
	if (fname[0] == '/') {
		strncpy(fullName, fname, MAX_PATH_LEN - 1);
		if (access(fullName, F_OK) == 0) {
			return fullName;
		}
	}

		/* Look in directories of PATH.  Use access() to find file there. */
	else {
		for (i = 0; i < num; i++) {
			// create fullName
			maxlen = MAX_PATH_LEN - 1;
			strncpy(fullName, dir[i], maxlen);
			maxlen -= strlen(dir[i]);
			strncat(fullName, "/", maxlen);
			maxlen -= 1;
			strncat(fullName, fname, maxlen);
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
	char *token;
	int i = 0;

	token = strtok(cmdLine, SEP);
	while (token != NULL && argc < MAX_ARGS) {
		cmd->argv[argc] = strdup(token);
		token = strtok(NULL, SEP);
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


void cleanUpJobs1(int *currNumbJobs, int *jobIDs, char **jobNames) {

//	while(1) {}

	// check if any jobs in the list have pids that no longer exist
	int jb = 0;
	for (; jb < *(currNumbJobs); jb++) {

		/* Note: even if a process finishes and exits, it's PID sometimes
		 * remains valid until the zombie process is reaped by its parent process
		 */


		kill(jobIDs[jb], 0);   // check if process is still alive
		if (errno == ESRCH) {

			jobNames[jb] = NULL;
			jobIDs[jb] = 0;
			currNumbJobs--;

		} else {
			//printf("Job is still valid: %d %s\n", jobIDs[jb], jobNames[jb]);
		}
	}
	// reposition job arrays to fill any null values between non-null values
	for (int m = jb; m < MAX_BACKGROUND_JOBS; m++) {    //

		if (jobNames[m] ==
		    NULL) {      // replace this NULL value with the next non-NULL value in the list, if one exists
			for (int n = m + 1; n < MAX_BACKGROUND_JOBS; n++) {

				if (jobNames[n] != NULL) {
					jobNames[m] = jobNames[n];
					jobIDs[m] = jobIDs[n];
					jobNames[n] = NULL;
					jobIDs[n] = 0;
					break;
				}
			}
		}
	}
}



/* TODO: figure out how to check if a job has completed or been terminated externally,
 * then automatically remove it from jobs list */

/** kill job
 * @param signal signal to pass to kill
 * @param pidToKill
 * @param currNmbJobs current number of jobs
 * @param jobIDs list of job ids
 * @param jobNames list of corresponding job names
 */
void killJob_(int signal, int pidToKill, int currNmbJobs, int *jobIDs, char **jobNames) {

	kill(pidToKill, signal);

	int jb = 0;
	for (; jb < currNmbJobs; jb++) {
		if (jobIDs[jb] == pidToKill) {
			jobNames[jb] = NULL;
			jobIDs[jb] = 0;
		}
	}
	// reposition job arrays to fill any null values between non-null values
	for (int m = jb; m < currNmbJobs; m++) {    //
		if (jobNames[m] == NULL) {      // replace this NULL value with the next non-NULL value, if there are any
			for (int n = m + 1; n < currNmbJobs; n++) {
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
}

/* TODO: figure out how to check if a job has completed or been terminated externally,
 * then automatically remove it from jobs list */

/** kill job
 * @param signal signal to pass to kill
 * @param pidToKill
 * @param currNmbJobs current number of jobs
 * @param jobIDs list of job ids
 * @param jobNames list of corresponding job names
 */
void killJob(int signal, int pidToKill, int *currNmbJobs, Job *jobs) {

	kill(pidToKill, signal);

	if (signal != 0) {

		int i = 0;
		for (; i < *(currNmbJobs); i++) {
			if (jobs[i].pid == pidToKill) {
				jobs[i].pid = 0;
				jobs[i].name = NULL;

				// only decrement currNmbJobs if the PID passed to kill matches a backgrounded task in the array
				(*currNmbJobs)--;  //  this is the same as *currNmbJobs = *(currNmbJobs) - 1;
				break;
			}
		}
	}
}

//void cleanUpJobs(void *jobs_struct_array) {  // for potential future multithreading
void cleanUpJobs(Job *jobs, int *currNumbJobs) {

	// check for completed jobs that should be removed, and simultaneously
	// reposition job array elements to make sure all null values are at end of array
	for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {

		errno = 0;
		kill(jobs[i].pid, 0);   // check if process is still alive

		if (errno == ESRCH) {
			printf("Deceased job:\t");
			printf("%d\t", jobs[i].pid);
			printf("%s\t", jobs[i].name);

			// todo
		}

		if (jobs[i].name == NULL) {      // replace this NULL value with the next non-NULL value, if one exists
			for (int m = i + 1; m < MAX_BACKGROUND_JOBS; m++) {
				if (jobs[m].name == NULL) {
					continue;
				}
				jobs[i].name = jobs[m].name;
				jobs[i].pid = jobs[m].pid;

				jobs[m].name = NULL;
				jobs[m].pid = 0;
				break;
			}
		}
	}
	//pthread_exit(NULL);
}

/**
 *
 * @param numb_args  initially 0, incremented with each valid arg and value used externally
 * @param argsForCommand
 * @param command
 * @param fullPath
 * @param input_index
 * @param argc
 * @return
 */
char** parseArgs(int *numb_args, char** argsForCommand, Command *command, char *fullPath, int *input_index, int argc) {
	// parse the argument flags

	int flagIndx = 0;

	for (; flagIndx < MAX_ARGS; flagIndx++) {
		argsForCommand[flagIndx] = NULL;
	}

	flagIndx = 0;     // initialize the first argument as the program's own name, which is how argv is expected
	argsForCommand[flagIndx++] = fullPath;

	// equivalent to *input_index = *(input_index) + 1; j = *(input_index)
	int j = ++(*input_index);
	while (j < argc && command->argv[j] != NULL) {         // pass all

		// assume all subsequent non-valid program names are arguments or flags to be passed to previous command
		if (command->argv[j][0] != '&') {
		//&& (command->argv[j][0] == '-' || lookupPath(command->argv[j], dirs, numDirs) == NULL)) {
			argsForCommand[flagIndx++] = command->argv[j++];
			input_index++;    // only increment i if the current argv value is a flag
		} else {
			break;
		}
	}

	*numb_args = flagIndx;

	return argsForCommand;
}


/*
  Run the shell
*/
int main(int argc, char *argv[]) {

	char *dirs[MAX_PATHS]; // list of dirs in environment
	int numDirs;
	char cmdline[LINE_LEN];

	numDirs = parsePath(dirs);
	char args[MAX_ARGS][MAX_ARG_LEN];

	char *input;
	Command *command;


	Job jobs[MAX_BACKGROUND_JOBS];

	// try to get username
	char *userName;
	if (((userName = getlogin()) == NULL) && ((userName = getenv("USER")) == NULL)) {
		userName = "user";  // default name if getting name was unsuccessful
	}


	/* NEW */
	for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
		jobs[i].pid = 0;
		jobs[i].name = NULL;
	}
	/* NEW */


	int currNumbJobs = 0;

	// ignore Ctrl-C while in Terminal
	struct sigaction sigHandler;
	sigHandler.sa_handler = ignoreSIGINT;
	sigemptyset(&sigHandler.sa_mask);
	sigHandler.sa_flags = 0;
	sigaction(SIGINT, &sigHandler, NULL);

	while (1) {

		char *currDir = (char *) malloc(MAX_PATH_LEN + 1);
		getcwd(currDir, MAX_PATH_LEN);

		command = malloc(sizeof(Command) + 1);
		input = (char *) malloc(MAX_ARGS * MAX_ARG_LEN + 1);

		printf("%s@bshell:%s$ ", userName, currDir);

		cleanUpJobs(jobs, &currNumbJobs);

		/* TODO: put this in a separate thread? */
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
		int runInBackground = FALSE;
		if (command->argv[argc - 1][0] == '&') {

			if (currNumbJobs < MAX_BACKGROUND_JOBS) {
				runInBackground = TRUE;
			} else {
				printf("Maximum number of background tasks already running, running job in foreground.\n");
			}
		}

		// parse input. compare against special commands first,
		// if unsuccessful then check if input is a valid executable
		for (int i = 0; i < argc; i++) {

			if (strncmp(command->argv[i], "cd", 2) == 0 && i < argc - 1) {

				if (chdir(command->argv[i + 1]) == 0) {       // Successfully changed directory
					i++;         // this combined with main 'i++' will skip the next argument, the path passed to cd
					continue;
				}
			} else if (strncmp(command->argv[i], "jobs", 4) == 0) {  // list jobs
				/*if (currNumbJobs == 0) {
					printf("No current jobs in background\n");
				} else {*/
				for (int l = 0; l < currNumbJobs; l++) {
					if (jobs[l].name != NULL) {
						printf("%d\t%s\n", jobs[l].pid, jobs[l].name);
					}
				}

				i++;
				continue;

			} else if (strncmp(command->argv[i], "kill", 4) == 0) {

				if (i < argc - 1) {  // if kill has at least one argument provided

					pid_t pidToKill;
					int sig = 15; // default signal

					// convert string termination signal to int
					if (i < argc - 2 && command->argv[i + 1][0] == '-') {

						char kSig[3];
						// this works for both 1 and 2 digit signal number strings
						sprintf(kSig, "%c%c", command->argv[i + 1][1], command->argv[i + 1][2]); // skip the hyphen
						sig = (int) strtol(kSig, NULL, 0);

						pidToKill = (pid_t) strtol(command->argv[i + 2], NULL, 10);

					} else { // no signal provided with kill command
						pidToKill = (pid_t) strtol(command->argv[i + 1], NULL, 10);
					}

					killJob(sig, pidToKill, &currNumbJobs, jobs);

					i += 2;  // increase input index past signal and pid words
				} else {
					printf("Not enough arguments passed to kill command\n");
				}
				continue;
			}

			// if program reaches this point, the command did not match any of the above functions
			// therefore, it is either a reference to a native executable or an invalid command 

			// check if the first argument is a valid executable filename
			char *fullPath = lookupPath(command->argv[i], dirs, numDirs);

			if (fullPath != NULL) {

				char *argsForCommand[MAX_ARGS];
				int numb_args = 0;

				parseArgs(&numb_args, argsForCommand, command, fullPath, &i, argc);

				pid_t pid = fork();

				if (pid == 0) {

					// run the executable, passing in the parsed argument flags
					execv(fullPath, argsForCommand);

					for (int k = 0; k < numb_args; k++) {
						free(argsForCommand[k]);
					}

					_exit(0);       // exit child process after the program finishes

				} else {

					if (runInBackground) {

						jobs[currNumbJobs].pid = pid;
						jobs[currNumbJobs].name = command->argv[0];   // TODO use the whole command, not just first word?

						currNumbJobs++;

					} else {
						waitpid(pid, NULL, 0);
					}
				}

			} else {
				fprintf(stderr, "Command not found: %s\n", fullPath);
			}
		}

		free(currDir);
		free(input);
		free(command);
	}

	free(dirs[0]);

	return 0;
}
