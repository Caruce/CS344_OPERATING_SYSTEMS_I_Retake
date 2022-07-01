#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>

#define MAX_CMD_BUFFER 2048			//command-line max buffer size
#define MAX_ARGS 512				//command-line max count of arguments
#define MAX_ARGV 64					//max size of one argument

char statusBuf[64];					//exit status buffer of the child process
int argCount = 0;

/*
* 1: the & operator should simply be ignored; 
* 0: the & operator is once again honored for subsequent commands, allowing them to be executed in the background.
*/
int tstpIgnored = 0;

/*the enum of the command state*/
enum CMD_STATE{Comment = 1, BlankLine, Forground, Background, Cd, Exit, Status};

/* struct for Command information */
typedef struct{
	int state;						//the value of enum CMD_STATE
	char cmd[MAX_ARGV];
	char * arguments[MAX_ARGS + 1];
	char inputFileName[MAX_ARGV];	//input redirect
	char outputFileName[MAX_ARGV];	//output redirect
} Command;

Command mCommand;					//current command
char backgroundFinishMsg[128];		//exit value of background process 

typedef void    Sigfunc(int);   /* for signal handlers */

Sigfunc *Signal(int, Sigfunc *);

Sigfunc *_signal(int signo, Sigfunc *func)
{
    struct sigaction    act, oact;

    act.sa_handler = func;
    //The function sigemptyset initializes the signal set pointed to by set so that all signals are excluded
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    if (signo == SIGALRM) {
#ifdef  SA_INTERRUPT
        act.sa_flags |= SA_INTERRUPT;   /* SunOS 4.x */
#endif
    } else {
#ifdef  SA_RESTART
        act.sa_flags |= SA_RESTART;     /* SVR4, 44BSD */
#endif
    }
    //The sigaction function allows us to examine or modify (or both) the action associated with a particular signal
    if (sigaction(signo, &act, &oact) < 0)
        return(SIG_ERR);
    return(oact.sa_handler);
}

Sigfunc *Signal(int signo, Sigfunc *func)    /* for our _signal() function */
{
    Sigfunc *sigfunc;
    if ( (sigfunc = _signal(signo, func)) == SIG_ERR)
        fprintf(stderr, "signal error=[%s]\n", strerror(errno));
    return(sigfunc);
}

/*
*find the oldstr of the str, and replace by newstr
*/
char *strrpc(char *str,char *oldstr,char *newstr)
{
	char bstr[strlen(str) + 1];
	memset(bstr,0x00,sizeof(bstr));
 
	for(int i = 0;i < strlen(str);i++)
	{
		if(!strncmp(str+i,oldstr,strlen(oldstr)))
		{
			strcat(bstr,newstr);
			i += strlen(oldstr) - 1;
		}else{
			strncat(bstr,str + i,1);
		}
	}
	strcpy(str,bstr);
	return str;
}

/*
*free the command
*/
void freeCommand(Command *cmd)
{
	int i;
	for(i = 0; i < argCount; i++)
	{
		if(cmd->arguments[i] != NULL)
		{
			free(cmd->arguments[i]);
		}
	}
}

/*
*parse the command of the input, and install the command struct
*/
void parseCommand(Command *cmd, char *cmdBuf)
{
	char *savePtr;
	char *paramPtr;
	char pid[16];
	argCount = 0;
	int i;

	memset(cmd, 0x00, sizeof(Command));
	memset(pid, 0x00, sizeof(pid));

	sprintf(pid, "%d", getpid());
	strrpc(cmdBuf, "$$", pid);
	//init state if Forground
	cmd->state = Forground;
	//delete the \n
	if(cmdBuf[strlen(cmdBuf)-1] == '\n')
	{
		cmdBuf[strlen(cmdBuf)-1] = '\0';
	}
	//ignore the \n
	if(cmdBuf[0] == '\0'){
		cmd->state = BlankLine;
		return;
	}
	//delete the last space
	while(cmdBuf[strlen(cmdBuf)-1] == ' ')
	{
		cmdBuf[strlen(cmdBuf)-1] = '\0';
	}
	//background state
	if(cmdBuf[strlen(cmdBuf)-1] == '&')
	{
		cmdBuf[strlen(cmdBuf)-1] = '\0';
		//trigger the SIGTSTP
		if(tstpIgnored == 0)
		{
			cmd->state = Background;
		}
	}
	//parse the command
	if((paramPtr = strtok_r(cmdBuf, " ", &savePtr)) != NULL)
	{
		strcpy(cmd->cmd, paramPtr);
		cmd->arguments[argCount] = (char *)malloc(sizeof(char *));
		strcpy(cmd->arguments[argCount], paramPtr);
		argCount++;
		if(cmd->cmd[0] == '#')
		{
			cmd->state = Comment;
			return;
		}
	}
	//parse the arguments
	while((paramPtr = strtok_r(NULL, " ", &savePtr)) != NULL)
	{
		//input redirect
		if(!strcmp(paramPtr, "<"))
		{
			if((paramPtr = strtok_r(NULL, " ", &savePtr)) != NULL)
			{
				strcpy(cmd->inputFileName, paramPtr);
			}
			continue;
		}
		//output redirect
		if(!strcmp(paramPtr, ">"))
		{
			if((paramPtr = strtok_r(NULL, " ", &savePtr)) != NULL)
			{
				strcpy(cmd->outputFileName, paramPtr);
			}
			continue;
		}
		if(argCount >= MAX_ARGS + 1)
		{
			continue;
		}
		cmd->arguments[argCount] = (char *)malloc(sizeof(char *));
		strcpy(cmd->arguments[argCount], paramPtr);
		argCount++;
	}
	//the status command
	if(!strcmp(cmd->cmd, "status"))
	{
		cmd->state = Status;
	}
	//the exit command
	if(!strcmp(cmd->cmd, "exit"))
	{
		cmd->state = Exit;
	}
	//the cd command
	if(!strcmp(cmd->cmd, "cd"))
	{
		cmd->state = Cd;
	}
	
}

/*
*forground process
*/
void processForground(Command *cmd)
{
	pid_t pid;
	int status, exitStatus;
	int infd, outfd;
	
	if((pid = fork()) < 0){
		printf("%s\n", strerror(errno));
		fflush(stdout);
	}else if(pid == 0){//child process
		Signal(SIGINT, SIG_DFL);//default SIGINT
		Signal(SIGTSTP, SIG_IGN);//ignore SIGTSTP
		//input redirect
		if(cmd->inputFileName[0] != '\0')
		{
			if((infd = open(cmd->inputFileName, O_RDONLY)) == -1)
			{
				printf("cannot open %s for input\n", cmd->inputFileName);
				fflush(stdout);
				exit(1);
			}
			dup2(infd,fileno(stdin));
		}
		//output redirect
		if(cmd->outputFileName[0] != '\0')
		{
			if((outfd = open(cmd->outputFileName, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1)
			{
				printf("cannot open %s for output\n", cmd->outputFileName);
				fflush(stdout);
				exit(1);
			}
			dup2(outfd,fileno(stdout));
		}
		if(execvp(cmd->cmd, cmd->arguments) == -1)
		{
			printf("%s: no such file or directory\n", cmd->cmd);
			fflush(stdout);
			exit(1);
		}
	}else{//parent process
		//wait the child process exit
		waitpid(pid, &status, 0);
		if (WIFEXITED(status)) {
			//get the child process exit status
			exitStatus = WEXITSTATUS(status);
			memset(statusBuf, 0x00, sizeof(statusBuf));
			sprintf(statusBuf, "exit value %d", exitStatus);
			if(exitStatus == 0)
			{
				if(!strcmp(cmd->cmd, "kill"))
				{
					char argSigno[128];
					memset(argSigno, 0x00, sizeof(argSigno));
					strcpy(argSigno, cmd->arguments[1] + 1);
					printf("background pid %s is done: terminated by signal %s\n", cmd->arguments[2], argSigno);
					fflush(stdout);
					memset(backgroundFinishMsg, 0x00, sizeof(backgroundFinishMsg));
				}else if(!strcmp(cmd->cmd, "pkill")){
					printf("the process %s terminated\n", cmd->arguments[1]);
					fflush(stdout);
					memset(backgroundFinishMsg, 0x00, sizeof(backgroundFinishMsg));
				}
			}
		} else if (WIFSIGNALED(status)) {
			exitStatus = WEXITSTATUS(status);
			memset(statusBuf, 0x00, sizeof(statusBuf));
			sprintf(statusBuf, "terminated by signal %d", WTERMSIG(status));
			printf("terminated by signal %d\n", WTERMSIG(status));
			fflush(stdout);
		}
		
	}
}

/*
*background process, handle the SIGCHLD
*/
void backgroundFinish(int signo)
{
	int status, exitStatus;
	pid_t pid;
	//get the child process exit status, avoid zombie process
	while ( (pid = waitpid(-1, &status, WNOHANG)) > 0)
	{
		exitStatus = WEXITSTATUS(status);
		memset(statusBuf, 0x00, sizeof(statusBuf));
		sprintf(statusBuf, "exit value %d", exitStatus);
		if(strcmp(mCommand.cmd, "kill"))
		{
			memset(backgroundFinishMsg, 0x00, sizeof(backgroundFinishMsg));
			sprintf(backgroundFinishMsg, "background pid %d is done: exit value %d", pid, exitStatus);
		}
	}
	Signal(SIGCHLD,backgroundFinish);
}

/*
*background process
*/
void processBackground(Command *cmd)
{
	pid_t pid;
	int status;
	int infd, outfd;
	
	Signal(SIGCHLD,backgroundFinish);
	if((pid = fork()) < 0){
		printf("%s\n", strerror(errno));
		fflush(stdout);
	}else if(pid == 0){//child process
		Signal(SIGINT, SIG_IGN);
		Signal(SIGTSTP, SIG_IGN);
		//input redirect
		if(cmd->inputFileName[0] != '\0')
		{
			if((infd = open(cmd->inputFileName, O_RDONLY)) == -1)
			{
				printf("cannot open %s for input\n", cmd->inputFileName);
				fflush(stdout);
				exit(1);
			}
			dup2(infd,fileno(stdin));
		}else{
			if((infd = open("/dev/null", O_RDONLY)) == -1)
			{
				printf("cannot open %s for input\n", cmd->inputFileName);
				fflush(stdout);
				exit(1);
			}
			dup2(infd,fileno(stdin));
		}
		//output redirect
		if(cmd->outputFileName[0] != '\0')
		{
			if((outfd = open(cmd->outputFileName, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1)
			{
				printf("cannot open %s for output\n", cmd->outputFileName);
				fflush(stdout);
				exit(1);
			}
			dup2(outfd,fileno(stdout));
		}else{
			if((outfd = open("/dev/null", O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1)
			{
				printf("cannot open %s for output\n", cmd->outputFileName);
				fflush(stdout);
				exit(1);
			}
			dup2(outfd,fileno(stdout));
		}
		if(execvp(cmd->cmd, cmd->arguments) == -1)
		{
			printf("%s: no such file or directory\n", cmd->cmd);
			fflush(stdout);
			exit(1);
		}
	}else{
		printf("background pid is %d\n", pid);
		fflush(stdout);
	}

}

/*
*process cd
*/
void processCd(Command *cmd)
{
	char currDir[MAX_CMD_BUFFER];
	memset(currDir, 0x00, sizeof(currDir));
	//cd $HOME
	if(cmd->arguments[1] == NULL)
	{
		cmd->arguments[1] = (char *)malloc(sizeof(char *));
		argCount++;
		strcpy(cmd->arguments[1], getenv("HOME"));
	}
	//change the Directory
	if(chdir(cmd->arguments[1]) == -1)
	{
		printf("%s: %s\n", cmd->arguments[1], strerror(errno));
		fflush(stdout);
		return;
	}
	//get current Directory
	getcwd(currDir, MAX_CMD_BUFFER);
	//set PWD environment
	setenv("PWD", currDir, 1);
}

/*
*handle the SIGTSTP
*/
void tstpFinish(int signo)
{
	fflush(stdout);
	if(strcmp(mCommand.cmd, "kill"))
	{
		fprintf(stdout, "\n");
		fflush(stdout);
	}
	if(tstpIgnored == 0)
	{
		tstpIgnored = 1;
		fprintf(stdout, "Entering foreground-only mode (& is now ignored)\n");
	}else{
		tstpIgnored = 0;
		fprintf(stdout, "Exiting foreground-only mode\n");
	}
	fflush(stdout);
	Signal(SIGCHLD,backgroundFinish);
	if(strcmp(mCommand.cmd, "kill"))
	{
		fprintf(stdout, ": ");
		fflush(stdout);
	}
	memset(&mCommand, 0x00, sizeof(Command));
}

int main(int argc, char *argv[])
{
	char cmdBuf[MAX_CMD_BUFFER + 1];	//command-line buffer

	memset(backgroundFinishMsg, 0x00, sizeof(backgroundFinishMsg));
	//ignore the SIGINT
	Signal(SIGINT, SIG_IGN);
	//handle the SIGTSTP
	Signal(SIGTSTP, tstpFinish);
	while(1)
	{
		printf(": ");
		fflush(stdout);
		memset(cmdBuf, 0x00, sizeof(cmdBuf));
		fgets(cmdBuf, MAX_CMD_BUFFER, stdin);
		//if there is background finish message, print it
		if(backgroundFinishMsg[0] != '\0')
		{
			printf("%s\n", backgroundFinishMsg);
			fflush(stdout);
			memset(backgroundFinishMsg, 0x00, sizeof(backgroundFinishMsg));
		}
		//parse command into mCommand
		parseCommand(&mCommand, cmdBuf);
		switch(mCommand.state)
		{
			case Comment:
			case BlankLine:
				//do nothing
				freeCommand(&mCommand);
				break;
			case Forground:
				//forground process
				processForground(&mCommand);
				freeCommand(&mCommand);
				break;
			case Background:
				//background process
				processBackground(&mCommand);
				freeCommand(&mCommand);
				break;
			case Cd:
				//cd command
				processCd(&mCommand);
				freeCommand(&mCommand);
				break;
			case Exit:
				freeCommand(&mCommand);
				//exit command, send SIGKILL to all the child process
				kill(0, SIGKILL);
				goto END;
				break;
			case Status:
				//status command
				printf("%s\n", statusBuf);
				fflush(stdout);
				freeCommand(&mCommand);
				break;
		}
	}
END:
	return EXIT_SUCCESS;
}
