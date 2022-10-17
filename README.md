# OSN Assignment 2

### This shell has been created by Adyansh Kakran - 202111020.
To run this shell, go to your linux terminal and enter `make` in the working directory of the Shell. Then enter `./main` to run the shell.

## The files in this submission of the shell are -
* `main.c` contains the main shell loop, all kinds of variable declarations and a function that processes all commands given to it.
* `functions.c` contains all the builtin commands of this shell including echo, pwd, ls, pinfo, discover, history and also the function to print the prompt on the screen.
* `utils.c` contains all other utility functions for the required commands and makes the code more modular.

## Functionalities
* This shell supports background as well as foreground processes using the `bg` and `fg` commands. Use the `jobs` command to find the job number of all the jobs in the background of the shell and use the above commands and the job number to bring them to the background, foreground or even send a signal to the process.
* Use the `TAB` key to autocomplete whatever file you were trying to access.
* Use the &uarr; and &darr; arrow keys to browse through your history and execute previously written commands.
* Use `Ctrl+Z` to push any foreground process to the background and `Ctrl+C` to interrupt any foreground process by sending it a `SIGINT` signal.
* Use `Ctrl+D` or `exit` to exit the shell.
