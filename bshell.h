/*
  header file for shell.  Adapted from Operating Systems, Nutt.
*/
//#define DEBUG 1
#define TRUE 1
#define FALSE 0
#define LINE_LEN 80

#define MAX_ARGS 64
#define MAX_ARG_LEN 16

#define MAX_PATHS 16
#define MAX_PATH_LEN 128 //96

#define SEP " \t\n"
#define WHITESPACE " .,\t\n"
#define DELIM ":"

#define PROMPT "$: "

/* Store commands in this structure */
typedef struct {
  int argc; // number of args
  char *argv[MAX_ARGS]; // args.  arg[0] is command
} Command;
