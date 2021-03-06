# BShell
BShell: the Bard Shell

BShell is a simple implementation of a Unix shell program. This was originally a lab project for my Computing Systems class at Bard college that I've continued working on for fun.

Currently implemented features:
- ls, cd
- jobs
- kill (with or without signals)
- '&' to run tasks in the background 
- `exit` to exit the shell (SIGINT (Ctrl-C) is caught and does not terminate the shell)
- all shell commands that run native executables (e.g. cat, grep, tar, etc)

Features to be implemented next:
- wildcard (`*`) expansion, e.g. `mv *.txt ..`
- I/O redirection (`>` and `<`), e.g. `cat file1 file2 > file3`
- piping (`|`), e.g. `cat example.txt | grep “keyword”`
- tab completion
- history


(Note this code will only run in Unix environments)
