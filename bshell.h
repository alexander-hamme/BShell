/*
  header file for shell.  Adapted from Operating Systems, Nutt.
*/

//#define DEBUG 1
#define LINE_LEN 80

#define MAX_ARGS 64
#define MAX_ARG_LEN 16

#define MAX_PATHS 16
#define MAX_PATH_LEN 128 //96

#define MAX_BACKGROUND_JOBS 10

#define SEP " \t\n"
#define WHITESPACE " .,\t\n"
#define DELIM ":"

#define PROMPT "$: "

/* Store commands in this structure */
typedef struct {
  int argc; // number of args
  char *argv[MAX_ARGS]; // args.  arg[0] is command
} Command;


typedef struct {
	int pid;
	char *name;
	int still_running;
} Job;
