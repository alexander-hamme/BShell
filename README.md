# BShell
BShell: the Bard Shell

BShell is a simple implementation of a Unix shell program. This was originally a lab project for my Computing Systems class at Bard college which I have kept working on for fun.

Currently Implemented Features:
- ls, cd
- jobs
- kill (with or without signals)
- '&' to run tasks in the background 
- `exit` to exit the shell (SIGINT (Ctrl-C) is caught and does not terminate the shell)
- all shell commands that run native executables (e.g. cat, grep, tar, etc)

Features to be Implemented Next:
- wildcard (`*`) expansion, e.g. `cat *.txt`
- I/O redirection (`>` and `<`), e.g. `ls ./ > filelist.txt`
- piping (`|`), e.g. `cat example.txt | grep “keyword”`
- history function

