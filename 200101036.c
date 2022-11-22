// C Program to design a minishell in Linux
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<readline/readline.h>
#include<readline/history.h>
#include<sys/stat.h>
#include<fcntl.h>
  int len=0;  
#define MAXI 1000 // max number of letters to be supported
#define MAXLT 100 // max number of commands to be supported
  
// Clearing the shell using escape sequences
#define clear() printf("\033[H\033[J")
  
// printing user on the terminal
void init_shell()
{
    clear();
    char* username = getenv("USER");
    printf("\n\n\nUSER is: @%s", username);
    printf("\n");
    sleep(1);
    clear();
}
/*void // free_mem(char** parsed,char* str)
{	
	free(str);
	for(int i=0;i<len;i++)
	{
		if(parsed[i]!=NULL)
		free(parsed[i]);
	
	}
}*/
  
// Function to take input from the terminal
//it uses readline function
int Input(char* str)
{
    char* spa;
  
    spa = readline("\n>>> ");
    if (strlen(spa) != 0) {
        add_history(spa);
        strcpy(str, spa);
        return 0;
    } else {
        return 1;
    }
}
  
//Current Directory is printed using this function.
void printDir()
{
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    printf("\nDir: %s\n", cwd);
}
  
// system commands are executed using this function
int execArgs(char **parsed,char* str)
{
  pid_t pid, wpid;
  int status;

  pid = fork();  //fork() is used to create a child process
  if (pid == 0) {
    // Child process
    if (execvp(parsed[0], parsed) == -1) {    //exec replaces the entire process with a new process
      perror("lsh");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    // Error forking
    perror("lsh");
  } else {
    // Parent process
    do {
      wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }
  return 1;
}
  
// system commands having pipes are executed using this function
//only single piping is taken care of in this function
void execArgsPiped(char** parsed, char** parsedpipe,char* str)
{
    // 0 is read end, 1 is write end
    int pipefd[2];    // to store file descriptors
    pid_t p1, p2;int stat;
    int std_out_dup = dup(1);
    
    if (pipe(pipefd) < 0) {
        printf("\nPipe could not be initialized");
        return;
    }
    p1 = fork();
    if (p1 < 0) {
        printf("\nCould not fork");
        return;
    }
  
    else if (p1 == 0) {
        // Child 1 executing..
        // It only needs to write at the write end
        dup2(pipefd[1],1);
        int ret = execvp(parsed[0],parsed);
        fflush(stdout);
        if(ret < 0)
        {
        	perror("lsh");
        	exit(1);
        
        }
        exit(0);
        }
     else {
        // Parent executing
        p2 = fork();
  
        if (p2 < 0) {
            printf("\nCould not fork");
            return;
        }
  
        // Child 2 executing..
        // It only needs to read at the read end
        else if (p2 == 0) {
            dup2(std_out_dup,1);
            close(pipefd[1]);dup2(pipefd[0],0);
            if (execvp(parsedpipe[0], parsedpipe) < 0) {
                printf("\nCould not execute command 2..");
                exit(1);
            }
            exit(0);
        } else {
            // parent executing, waiting for two children
            close(pipefd[0]);
            close(pipefd[1]);
            waitpid(p1,&stat,0);
            waitpid(p2,&stat,0);
        }
    }
    
 
}
  
// Help command
void openHelp()
{
    puts("\n***WELCOME TO MY SHELL HELP***"
        "\nList of Commands supported:"
        "\n>cd"
        "\n>echo"
        "\n>exit or quit or x"
        "\n>all other general commands available in UNIX shell"
        );
    return;
}
  
// builtin commands are executed using this function
int builtinhandler(char** parsed)
{
    int NoOfOwnCmds = 8, i, switchOwnArg = 0;
    char* ListOfOwnCmds[NoOfOwnCmds];
    char* username;
  
    ListOfOwnCmds[0] = "exit";
    ListOfOwnCmds[1] = "cd";
    ListOfOwnCmds[2] = "help";
    ListOfOwnCmds[3] = "hello";
    ListOfOwnCmds[4] = "echo";
    ListOfOwnCmds[5] = "quit";
    ListOfOwnCmds[6] = "x";
    ListOfOwnCmds[7] = "setenv";
  
    for (i = 0; i < NoOfOwnCmds; i++) {
        if (strcmp(parsed[0], ListOfOwnCmds[i]) == 0) {
            switchOwnArg = i + 1;
            break;
        }
    }
    switch (switchOwnArg) {
    case 1:case 6:case 7:    // all three commands quit,exit,x are supposed to do the same thing 
        printf("\nGoodbye\n");remove("/tmp/history.txt");
        exit(0);
    case 2:
        if(parsed[1]!=NULL)     // checking if any argument is given with cd
        	chdir(parsed[1]);   // changing directory
        else
        	chdir(getenv("HOME"));   // if only cd is given without any arguments then directory is changed to HOME
        return 1;
    case 3:
        openHelp();
        return 1;
    case 4:
        username = getenv("USER");   //getenv gives the value of the environmental variables
        printf("\nHello %s.\n",username);
        return 1;
    case 5:
    	if((parsed[1][0])=='$')   // to print the values of env. variables using echo $var
    	   {
    	   	const char* t = (parsed[1]+1);
    	   	const char* s = getenv(t);
		if(s!=NULL)
			printf("%s\n",s);
		else
		   perror("lsh");
    	   }
    	   else
    	   {                                  
    	   	printf("%s",parsed[1]);
    	   }
    	   return 1;
    case 8:
    	if(strcmp(parsed[2],"=")==0)
     	{
     		setenv(parsed[1],parsed[3],1);return 1;
     	}
     	else
     		perror("lsh");
    default:
        break;
    }
  
    return 0;
}
  
// function to find if there is a piping in the given command
int parsePipe(char* str, char** arrpiped)   // to check if there is a pipe in the given command,if yes,then separating the command before pipe and after pipe
{
    int i;
    for (i = 0; i < 2; i++) {
        arrpiped[i] = strsep(&str, "|");
        if (arrpiped[i] == NULL)
            break;
    }
  
    if (arrpiped[1] == NULL)
        return 0; // returns zero if no pipe is found.
    else {
        return 1;
    }
}
void execredir1(char** parsed,int len,char* str) //function to execute commands having output redirection operation (>)
{
	pid_t p1;int status;
	p1 = fork();
	if(p1==0)
	{
		int fd1 ;
        	if ((fd1 = creat(parsed[len-1] , 0644)) < 0) {
         	   perror("Couldn't open the output file");
          	  exit(0);
           	 }
           	 dup2(fd1, STDOUT_FILENO);parsed[len-2]=NULL;
           	 if (execvp(parsed[0], parsed) < 0) {
                printf("\nCould not execute command 2..");
                exit(0);
                }
                close(fd1);
        }
        else if((p1) < 0)
    	{     
        	printf("fork() failed!\n");
        	exit(1);
    	}

    	else {                                  /* for the parent:      */

        	while (!(wait(&status) == p1)) ; // good coding to avoid race_conditions(errors) 
    	}
    

}

    //if '<' char was found in string inputted by user
  /*  if(in)
    {   

        // fdo is file-descriptor
        int fd0;
        if ((fd0 = open(input, O_RDONLY, 0)) < 0) {
            perror("Couldn't open input file");
            exit(0);
        }           
        // dup2() copies content of fdo in input of preceeding file
        dup2(fd0, 0); // STDIN_FILENO here can be replaced by 0 

        close(fd0); // necessary
    }

    //if '>' char was found in string inputted by user 
    if (out)
    {

        int fd1 ;
        if ((fd1 = creat(output , 0644)) < 0) {
            perror("Couldn't open the output file");
            exit(0);
        }           

        dup2(fd1, STDOUT_FILENO); // 1 here can be replaced by STDOUT_FILENO
        close(fd1);
    }*/

void execredir2(char** parsed,int len,char* str) //function to execute commands having input redirection operation (<)
{
	pid_t p1;int status;
	p1 = fork();
	if(p1==0)
	{
		int fd0 ;
        	if ((fd0 = open(parsed[len-1], O_RDONLY, 0)) < 0) {
            		perror("Couldn't open input file");
            		exit(0);
        	}        
        	dup2(fd0, 0);
        	parsed[len-2]=NULL; // STDIN_FILENO here can be replaced by 0 
       	close(fd0);    
           	 
           	 if (execvp(parsed[0], parsed) < 0) {
                printf("\nCould not execute command 2..");
                exit(0);
                }
        }
        else if((p1) < 0)
    	{     
        	printf("fork() failed!\n");
        	exit(1);
    	}

    	else {                                  /* for the parent:      */

        	while (!(wait(&status) == p1)) ; // good coding to avoid race_conditions(errors) 
    	}
    	
	
	
	
}
int execredir3(char** parsed,int len,char* str) //function to execute commands having append operation (>>)
{
	pid_t p1;int status;
	p1 = fork();
	if(p1==0)
	{
			int std_out_dup = dup(1);
			int fd = open(parsed[len-1], O_WRONLY | O_APPEND | O_CREAT, 0666);
			dup2(fd, 1);
			parsed[len-2] = NULL;
			if (execvp(parsed[0], parsed) < 0) {
                		printf("\nCould not execute command 2..");
                		exit(0);
                	}
			fflush(stdout);
			close(fd);
			dup2(std_out_dup, 1);
        }
        else if((p1) < 0)
    	{     
        	printf("fork() failed!\n");
        	exit(1);
    	}

    	else {                                  /* for the parent:      */

        	while (!(wait(&status) == p1)) ; // good coding to avoid race_conditions(errors) 
    	}
    	
	
	
}


// command words are parsed using this function,i.e., separated w.r.t spaces
int spaceparser(char* str, char** parsed)
{
    int i;
  
    for (i = 0; i < MAXLT; i++) {
        parsed[i] = strsep(&str, " ");
  
        if (parsed[i] == NULL)
            break;
        if (strlen(parsed[i]) == 0)
            i--;
    }
    len=i;
    return i;
}
  
int processString(char* str, char** parsed, char** parsedpipe)  //function to process the input command string 
{
  
    char* arrpiped[2];char* arredir1[2];char* arredir2[2];
    int piped = 0;
  
    piped = parsePipe(str, arrpiped);
    
  
    if (piped) {
        spaceparser(arrpiped[0], parsed);
        spaceparser(arrpiped[1], parsedpipe);
        piped=0;
  
    } else {
    		piped = spaceparser(str,parsed);
        }
    if (builtinhandler(parsed))      // checking if the command given is present in the on of the internal commands
        {
        	
        	return 0;
        }
    else
        return 1 + piped;
}

  
int main()
{
    char inputString[MAXI], *parsedArgs[MAXLT];
    char* parsedArgsPiped[MAXLT];
    int execFlag = 0;
    init_shell(); 
  
    while (1) {
        // print shell line
        printDir();
        // take input
        if (Input(inputString))
            continue;
        // process
        FILE *q = fopen("/tmp/history.txt","a+");
        fprintf(q,"%s\n",inputString);fclose(q); //storing all the commands entered in the /tmp/history.txt file
        if(strcmp(inputString,"history")==0)
        {
        	strcpy(inputString,"cat /tmp/history.txt");   // if the command entered is history then changing that command to print the history file
        }
        execFlag = processString(inputString,
        parsedArgs, parsedArgsPiped);
        // execflag returns zero if there is no command
        // or it is a builtin command,
        // 1 if it is a simple command
        // 2 if it is including a pipe.
        // execute
        if(execFlag==1)
        	execArgsPiped(parsedArgs, parsedArgsPiped,inputString); 
        if(execFlag>1)
        {	int b = 0;
			for(int i=0;i<len;i++)		//checking if the command has any redirection symbol
			{
				if(strcmp(parsedArgs[i], ">") == 0)   
				{  
					b = 1;
				}
				if(strcmp(parsedArgs[i], "<") == 0)
				{
					b = 2;
					
				}
				if(strcmp(parsedArgs[i], ">>") == 0)
				{
					b = 3;
				}
			}
			switch(b)
			{
				case 1: execredir1(parsedArgs,len,inputString);break;
				case 2: execredir2(parsedArgs,len,inputString);break;
				case 3: execredir3(parsedArgs,len,inputString);break;
				case 0: execArgs(parsedArgs,inputString);break;
				default : break;
			}
			
	}

    }
    return 0;
}
