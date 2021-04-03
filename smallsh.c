/*
** Name: Conner Foster (932777502)
** Date: Oct 28, 2020
** Class: CS 344_001
** Program: This assignment required me to write my own shell in C. I had to implement a subset of features of well-known shells. This required heavy use of fork() 
** and execvp. The program requirements were " Provide a prompt for running commands, Handle blank lines and comments, which are lines beginning with the # character, 
** Provide expansion for the variable $$, Execute 3 commands exit, cd, and status via code built into the shell, Execute other commands by creating new processes using 
** a function from the exec family of functions, Support input and output redirection, Support running commands in foreground and background processes, Implement custom 
** handlers for 2 signals, SIGINT and SIGTSTP." The user needs to be able to enter numerous commands and the program will have to be able to read each of those commands
** individually to understand what the user is trying to accomplish. I was able to succsessfully do this by turning the user input into a 2d array after tokenizing the 
** inputed commands. After that, the command execution is handled mostly by execvp and the three built in commands: exit, cd, and status.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h> //for date and time generation
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <fcntl.h>

void getInput(int status);
int FG = 0;

void redirOut(char* arguments)
{
	// Open target file
	int targetFD = open(arguments, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (targetFD == -1) {
		perror("target open()");
		exit(1);
	}

	// Redirect stdout to target file
	int result = dup2(targetFD, 1);
	close(targetFD);
	if (result == -1) {
		perror("target dup2()");
		exit(2);
	}
}


void redirIn(char* arguments)
{
	// Open source file
	int sourceFD = open(arguments, O_RDONLY);
	if (sourceFD == -1) {
		perror("cannot open badfile for input\n");
		exit(1);
	}

	// Redirect stdin to source file
	int result = dup2(sourceFD, 0);
	if (result == -1) {
		perror("source dup2()\n");
		exit(2);
	}
}


int greaterOrLesser(char** arguments)
{
	if (strcmp(arguments[1], ">") == 0 && arguments[3] == NULL) //if user wants to redirect output to file
	{
		redirOut(arguments[2]);
		return 1;
	}
	if (strcmp(arguments[1], "<") == 0 && arguments[3] == NULL) //if user wants to redirect input to terminal
	{
		redirIn(arguments[2]);
		return 1;
	}
	if (strcmp(arguments[1], "<") == 0 && strcmp(arguments[3], ">") == 0) //if user wants to redirect the input into a file
	{
		redirIn(arguments[2]);
		redirOut(arguments[4]);
		return 1;
	}
	if (strcmp(arguments[1], ">") == 0 && strcmp(arguments[3], "<") == 0) //if user want to redirect output into file and then redirect the input to the terminal
	{
		redirOut(arguments[2]);
		redirIn(arguments[4]);
		return 1;
	}
	return 0;
}


void check$$(char** arguments)
{
	int counter = 0; //used to parse through arguments
	char* mypid = malloc(10); //alocate memory for pid alue
	int pid = getppid();
	sprintf(mypid, "%d", pid);

	while (arguments[counter] != NULL)
	{
		char* str = strdup(arguments[counter]); //copy the argument into the string
		for (int i = 0; i < (strlen(arguments[counter]) - 1); i++)  //loop through the string
		{
			if (str[i] == '$' && str[i + 1] == '$') { //if $$ is found
				arguments[counter] = mypid; //set the $$ to the pid
				break;
			}
		}
		counter++;
	}

}


int checkAmp (char** arguments)
{
	int numArgs = 0; //initialize int to store the number of arguments the user has input
	while (arguments[numArgs] != NULL) //while there is an argument to read
	{
		numArgs++; //increment number of found arguments
	}
	
	if (strcmp(arguments[numArgs - 1], "&") == 0) //if the last argument is "&"
	{
		arguments[numArgs - 1] = NULL; //remove the & from the input so when the arguments are ran there is no issue.
		return 1;
	}
	else //else & at end not found
		return 2;
	return 0; //if no & was found
}


void runCmd(char** arguments, int* status)
{
	int wstatus;
	signal(SIGINT, SIG_IGN);

	pid_t spawnpid = -5, w;
	//if fork is successful, the value of spawnpid will be 0 in the child, the child's pid in the parent
	spawnpid = fork();
	switch (spawnpid) {
	case -1:
		//code in this branch will be exected by the parent when fork() fails and the creation of child process fails as well
		perror("fork() failed!");
		exit(1);
		break;
	case 0:
		//spawnpid is 0. This means the child will execute the code in this branch
		signal(SIGINT, SIG_DFL);
		checkAmp(arguments); //before commands are processed, check for "&" in commands
		//check$$(arguments); //before commands are processed, check for "$$" in commands

		if (arguments[1] != NULL)
		{
			if (greaterOrLesser(arguments) == 1) //if compare operator is found
			{
				execlp(arguments[0], arguments[0], NULL); //executes only the command (the first word the user inputs)
				*status = 1; //if execvp didnt run (because of bad input) then thi sets status equal to 1 to indicate bad input and that the commands were not ran
			}
			else
			{
				execvp(arguments[0], arguments); //executes all commands from inputs
				*status = 1; //if execvp didnt run (because of bad input) then thi sets status equal to 1 to indicate bad input and that the commands were not ran
			}
		}
		else 
		{
			execlp(arguments[0], arguments[0], NULL); //executes only the command (the first word the user inputs)
			printf("No such file or directory\n"); //if execvp didnt run (because of bad input) then this error message will print instead
			*status = 1; //set status equal to 1 to indicate bad input and that the commands were not ran
		}
		break;
	default:
		if (checkAmp(arguments) == 1 && FG == 0) //if "&" is found as last argument
		{
			printf("background pid is %d\n", getpid()); //print background pid to screen
			spawnpid = waitpid(spawnpid, &wstatus, WNOHANG);
			if ((spawnpid = waitpid(spawnpid, &wstatus, WNOHANG)) > 0) //if child process has been terminated
			{
				printf("background pid %d is done: terminated by signal %d\n", getpid(), WTERMSIG(wstatus));
			}
		}
		else
		{
			// spawnpid is the pid of the child. This means the parent will execute the code in this branch
			do
			{
				w = waitpid(spawnpid, &wstatus, WUNTRACED | WCONTINUED);
				if (w == -1) {
					perror("waitpid");
					exit(EXIT_FAILURE);
				}

				//these if statements catch errors with the file processing using built in catch commands
				if (WIFEXITED(wstatus)) {
					*status = WEXITSTATUS(wstatus); //sets status = 1 when an error is caught
					//printf("exited, status=%d\n", WEXITSTATUS(wstatus));
				}
				else if (WIFSIGNALED(wstatus)) {
					printf("killed by signal %d\n", WTERMSIG(wstatus));
				}
				else if (WIFSTOPPED(wstatus)) {
					printf("\nstopped by signal %d\n", WSTOPSIG(wstatus));
				}
				else if (WIFCONTINUED(wstatus)) {
					printf("continued\n");
				}
			} while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus)); //continue the loop while an error was caught
			break;
		}
	}
}


void stat(int status) //print the status of the last process
{
	printf("exit value %d\n", status);
}


void changeDir(char** arguments) //changes current directory
{
	if (arguments[1] == NULL) //if user enters cd with out a specified file then change to HOME diectory
	{
		char* home = getenv("HOME"); //searches the environment list for "HOME" and sets that to string "home"
		chdir(home); //changed directory to the environment list named "HOME"
	}
	else
		chdir(arguments[1]); //change directory to the directory that was inputed
}


void quit() //quits out of program
{
	exit(0);
}


void checkInput(char** arguments, int status) //checks what the user input and correctly directs the user and the information to the correct next functions
{
	check$$(arguments);

	if (strcmp(arguments[0], "status") == 0) //if user command is "status"
		stat(status);
	else if (strcmp(arguments[0], "cd") == 0) //if user command is "cd"
		changeDir(arguments);
	else if (strcmp(arguments[0], "exit") == 0) //if user command is "exit"
		quit();
	else if (strcmp(arguments[0], "#") == 0) //if user types a comment, skip processing the data
		getInput(status);
	else //all other commands use the execvp() function which is processed in runCmd()
		runCmd(arguments, &status);

	getInput(status); //if input does not meet any of those requirements, get new input
}


void foregroundSwitch() //changes to variable "FG" (the forground tracker) and turns forground mode by settting the tracker to 0 or 1
{
	if (FG == 1) //if the forground process has ended
	{
		char* print = "\nEnded foreground only mode\n";
		write(STDOUT_FILENO, print, 28); //print out the statement above
		FG = 0; //set foreground tracker to "off"
	}
	else //if forground mode is started
	{
		char* print = "\nStarted foreground only mode\n";
		write(STDOUT_FILENO, print, 30); //print out the statement above
		FG = 1; //set foreground tracker to "on"
	}
}


void getInput(int status) //gets user input
{
	//this section of code is used to initialize signals
	struct sigaction SIGTSTP_action = { 0 }; // initialize SIGINT_action struct to be empty
	SIGTSTP_action.sa_handler = foregroundSwitch; // register handle_SIGINT as the signal handler
	sigfillset(&SIGTSTP_action.sa_mask);   // block all catchable signals while handle_SIGINT is running
	sigaction(SIGTSTP, &SIGTSTP_action, NULL); //install our signal handler
	
	//this section of code is used to get the user input and fill a 2d array with the arguments
	int looper = 0;
	while (looper != 1)
	{
		char input[2048]; //maximum number of charecter inputs allowed is 2048
		char* blank[1] = { }; //variable to check that user input isnt blank
		printf(": ");
		fgets(input, 2049, stdin); //fgets rads untill (n-1) so the referenced int is 2049

		char* saveptr = input;
		char* arguments[513]; //user can input a maximum of 512 arguments, so 513 to account for the command input
		char* token;
		int i = 0; //used to parse through array

		i = 0;
		while ((token = strtok_r(saveptr, " \n", &saveptr))) //runs for every time it finds desired charecters (" " and "\n")
		{
			arguments[i] = token; //fill arguments array
			i++;
		}
		i++;
		arguments[i] = NULL; //NULL terminate the argument array.

		if (arguments[0] == blank[0]) //if user input is blank
			getInput(status); //reset input
		else
			checkInput(arguments, status);
	}
}


int main() //starts the program
{
	int status = 0; //status is declared in main as it is changed and used in many functions
	getInput(status);
}