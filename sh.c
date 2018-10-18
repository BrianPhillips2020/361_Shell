/*

CISC361 Shell

Marc Bolinas
Brian Phillips


 */

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <dirent.h>
#include <sys/types.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <wordexp.h>
#include <pthread.h>
#include <utmpx.h>
#include "sh.h"

//external global var from main.c, used for signal handling
extern pid_t childpid;

struct strlist *watchuserhead;
struct maillist *watchmailhead;
struct children *childhead = NULL;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int argcount = 0;

int sh( int argc, char **argv, char **envp )
{
  char *prompt = calloc(PROMPTMAX, sizeof(char));
  char *commandline = calloc(MAX_CANON, sizeof(char));
  char *command, *arg, *currentdir, *previousdir, *pwd, *owd;
  char **args = calloc(MAXARGS, sizeof(char*));
  int uid, i, go = 1, noclobber = 0, watchthread = 0, mailthread = 0;
  int sout = 0, serr = 0, sin = 0;
  struct passwd *password_entry;
  char *homedir;
  struct pathelement *pathlist;

  //initializations, such as getting home dir, pathlist, etc
  #include "init.c"

  //a whole bunch of different list structures
  struct history *histhead = NULL;
  struct history *histtail = NULL;
  int history_length = 10;
  int current_length = 0;
  watchuserhead = NULL;
  watchmailhead = NULL;
  struct alias_entry *ahead = NULL;
  struct alias_entry *atail = NULL;


  
  int buffersize = PROMPTMAX;
  char buffer[buffersize];
  char tempbuffer[buffersize];

  while (go)
    {
      int input_error = 0;//if the user didn't input anything or inputted something incorrect
      int expanded = 0;//if we expanded a * or ?
      printf("\n%s%s >> ", prompt, currentdir);
     

      /* get command line and process */
      if(fgets(buffer, buffersize, stdin) == NULL){
	//when ^D is entered, EOF status gets put on stdin
	//clearerr() removes that status and lets it continue as normal
	printf("\nIntercepted ^D\n");
	clearerr(stdin);
	continue;
      }
      else{
	buffer[(int) strlen(buffer) - 1] = '\0';
      }

      //add commandline to history (so long as it's not an emptry string)
      if((int) strlen(buffer) != 0){
	#include "addhistory.c"
      }


      strcpy(tempbuffer, buffer);
      char *tkn;
      tkn = strtok(tempbuffer, " ");

      //if we find an alias, overwrite buffer with the alias's command
      //the reason we need tempbuffer is because strtok() messes up
      //when called twice on the same string
      struct alias_entry *trav = ahead;
      int found_alias = 0;
      while(trav != NULL && found_alias == 0){
	if(strcmp(tkn, trav->key) == 0){
	  strcpy(buffer, trav->command);
	  printf("Executing alias [%s]\n", buffer);
	  found_alias = 1;
	}
	trav = trav->next;
      }

      //parses command line, strcpy() for command and all the args
      #include "parsecommandline.c"
	

      int redirect = 0;
      char *redir_type;
      char *redir_dest;

      //saving stdio to revert back after redirection
      int save_stdin = dup(STDIN_FILENO);
      int save_stdout = dup(STDOUT_FILENO);
      int save_stderr = dup(STDERR_FILENO);
      int fileid;


      for(int j = 1; j < argcount; j++){
	if(strcmp(args[j], ">") == 0 || strcmp(args[j], ">&") == 0 || strcmp(args[j], ">>") == 0 || strcmp(args[j], ">>&") == 0 || strcmp(args[j], "<") == 0){
	  redirect = j;
	  redir_type = args[j];
	  redir_dest = args[j + 1];
	}
      }

      if(redirect != 0){
	struct stat sttmp;

	/*
	  find out what kind of redirection we're doing, if we're allowed to do it
	  then open necessary file, close necessary stdio, etc.
	  
	  each else-if block is one type of redirection
	 */
	if(strcmp(redir_type, ">") == 0){
	  if(noclobber == 1 && stat(redir_dest, &sttmp) == 0){
	    printf("Noclobber is preventing '>' for %s\n", redir_dest);
	    input_error = 1;
	  }
	  else{
	    fileid = open(redir_dest, O_CREAT|O_TRUNC|O_WRONLY, 0666);
	    close(STDOUT_FILENO);
	    dup(fileid);
	    close(fileid);
	    sout = 1;
	  }
	}
	else if(strcmp(redir_type, ">&") == 0){
	  if(noclobber == 1 && stat(redir_dest, &sttmp) == 0){
	    printf("Noclobber is preventing '>&' for %s\n", redir_dest);
	    input_error = 1;
	  }
	  else{
	    fileid = open(redir_dest, O_CREAT|O_TRUNC|O_WRONLY, 0666);
	    close(STDOUT_FILENO);
	    dup(fileid);
	    close(STDERR_FILENO);
	    dup(fileid);
	    close(fileid);
	    sout = 1;
	    serr = 1;
	  }
	}
	else if(strcmp(redir_type, ">>") == 0){
	  if(noclobber == 1 && stat(redir_dest, &sttmp) != 0){
	    printf("Noclobber is preventing '>>' for non-existent file %s\n", redir_dest);
	    input_error = 1;
	  }
	  else{
	    fileid = open(redir_dest, O_CREAT|O_WRONLY|O_APPEND, 0666);
	    close(STDOUT_FILENO);
	    dup(fileid);
	    close(fileid);
	    sout = 1;
	  }
	}
	else if(strcmp(redir_type, ">>&") == 0){
	  if(noclobber == 1 && stat(redir_dest, &sttmp) != 0){
	    printf("Noclobber is preventing '>>&' for non-existent file %s\n", redir_dest);
	    input_error = 1;
	  }
	  else{
	    fileid = open(redir_dest, O_CREAT|O_WRONLY|O_APPEND, 0666);
	    close(STDOUT_FILENO);
	    dup(fileid);
	    close(STDERR_FILENO);
	    dup(fileid);
	    close(fileid);
	    sout = 1;
	    serr = 1;
	  }
	}
	else{
	  if(stat(redir_dest, &sttmp) < 0){
	    perror("Error accessing specified file");
	    input_error = 1;
	  }
	  else{
	    fileid = open(redir_dest, O_RDONLY);
	    close(STDIN_FILENO);
	    dup(fileid);
	    close(fileid);
	    sin = 1;
	  }
	}
	//set the redirection command, and file name, to NULL
	args[redirect] = NULL;
	args[redirect + 1] = NULL;

      }


      //support for pipe, |
      int pipel = -1;
      for(int j = 0; j < argcount; j++){
	if(args[j] != NULL && strcmp(args[j], "|") == 0){
	  pipel = j;//found index of pipe
	}
      }

      if(pipel == 0 || pipel == argcount){
	pipel = 0;
	input_error = 1;
	//the pipe can't be the first or last arg
      }

      char **leftargs = calloc(MAXARGS, sizeof(char*));
      char **rightargs = calloc(MAXARGS, sizeof(char*));
      pid_t leftchild = -1, rightchild = -1;
      

      if(pipel != 0 && pipel != -1){
	/*
	  split up args into two parts:
	  arguments on the left side of |, and
	  arguments on the right side of |
	 */
	for(int j = 0; j < pipel; j++){
	  leftargs[j] = malloc(1 + (sizeof(char) * strlen(args[j])));
	  strcpy(leftargs[j],args[j]);
	}
	int k = 0;
	for(int j = pipel + 1; j < argcount; j++){
	  rightargs[k] = malloc(1 + (sizeof(char) * strlen(args[j])));
	  strcpy(rightargs[k],args[j]);
	  k++;
	}

	//creating our pipe for IPC
	int ipc[2];
	if(pipe(ipc) != 0){
	  perror("Error when parsing '|', ");
	  exit(0);
	}


	leftchild = fork();
	if(leftchild == 0){
	  /*
	    left child gets its args and command,
	    sets it's stdout to be the pipe's input
	   */
	  args = leftargs;
	  go = 0;
	  close(STDOUT_FILENO);
	  dup(ipc[1]);
	  close(ipc[0]);
	}
	else{
	  rightchild = fork();
	  if(rightchild == 0){
	    /*
	      right child gets its args and command,
	      sets it's stdin to be the pipe's output
	     */
	    args = rightargs;
	    command = NULL;
	    free(command);
	    command = malloc(1 + (sizeof(char) * strlen(args[0])));
	    strcpy(command, args[0]);
	    go = 0;
	    close(STDIN_FILENO);
	    dup(ipc[0]);
	    close(ipc[1]);
	  }
	  else{
	    input_error = 1;//input_error is 1 so the parent doesn't run a command this time
	    
	    close(ipc[0]);
	    close(ipc[1]);
	    
	    //add left child to the list of children, because we don't use a blocking wait
	    struct children *tmp;
	    tmp = malloc(sizeof(struct children));
	    tmp->pid = leftchild;
	    tmp->next = NULL;
	    if(childhead == NULL){
	      childhead = tmp;
	    }
	    else{
	      tmp->next = childhead;
	      childhead = tmp;
	    }
	    
	    waitpid(rightchild, NULL, 0);
	  }
	}

      }




      
      //check for each builtin command
      //some commands are separate functions because they're long
      if(input_error == 1){
	//if the user didn't input anything or didn't input correctly, don't do anything
      }
      else if(strcmp(command, "exit") == 0){

	go = 0;
	printf("Closing shell...\n\n\n");
      }
      else if(strcmp(command, "cd") == 0){

	if(expanded) args[2] = NULL;//if we expanded a * or ?, use the first result, args[1]
	cd(command, args, homedir, currentdir, previousdir);
      }
      else if(strcmp(command, "pwd") == 0){

	char *tmp;
	tmp = getcwd(NULL, 0);
	printf("[%s]\n", tmp);
	free(tmp);
      }
      else if(strcmp(command, "list") == 0){

	list(command, args, currentdir);
      }
      else if(strcmp(command, "prompt") == 0){

	#include "prompt.c"
      }
      else if(strcmp(command, "pid") == 0){

	printf("PID: %d\n", getpid());
      }
      else if(strcmp(command, "kill") == 0){

	killsig(command, args);
      }
      else if(strcmp(command, "which") == 0){

	#include "which.c"
      }
      else if(strcmp(command, "where") == 0){

	#include "where.c"
      }
      else if(strcmp(command, "history") == 0){

	#include "history.c"
      }
      else if(strcmp(command, "printenv") == 0){

	printenv(args, envp);
      }
      else if(strcmp(command, "setenv") == 0){

	#include "setenv.c"
      }
      else if(strcmp(command, "watchmail") == 0){

	#include "watchmail.c"
      }
      else if(strcmp(command, "watchuser") == 0){

	#include "watchuser.c"
      }
      else if(strcmp(command, "noclobber") == 0){

	if(noclobber == 0){
	  noclobber = 1;
	  printf("Noclobber is now on\n");
	}
	else{
	  noclobber = 0;
	  printf("Noclobber is now off\n");
	}
      }      
      else if(strcmp(command, "alias") == 0){

	#include "alias.c"
      }
      else{//if it's not a builtin command, it's either an external command or not valid

	execute_command(command, args, envp, pathlist);

      }

      /*
	if we modified any stdio through redirection, restore them to
	their default
       */
      if(sin == 1){
	sin = 0;
	close(fileid);
	dup2(save_stdin, 0);
      }
      if(sout == 1){
	sout = 0;
	close(fileid);
	dup2(save_stdout, 1);
      }
      if(serr == 1){
	serr = 0;
	close(fileid);
	dup2(save_stderr, 2);
      }

      //call non-blocking wait() for each child process
      struct children *tmp;
      tmp = childhead;
      while(tmp->next != NULL){

	int p = waitpid(tmp->next->pid, NULL, WNOHANG);

	if(p != 0){
	  printf("(Shell: found dead child, pid=%d)\n", p);
	  tmp->next = tmp->next->next;
	}
	else{
	  tmp = tmp->next;
	}
      }

      //shell sub-processes called via |
      if(leftchild == 0 || rightchild == 0){
	//printf("child process is now exiting\n");
	exit(0);
      }

      //freeing command and args for realloc
      command = NULL;
      free(command);
      for(int j = 0; j <= i; j++){
       	args[j] = NULL;
	free(args[j]);
      }
    }

  //free() everything on shell exit
  free(prompt);
  free(commandline);
  free(args);
  free(currentdir);
  if(previousdir != NULL) free(previousdir);
  free(owd);
  free(pwd);
  free(arg);
  free(command);


  struct history *tmp = histhead;
  while(tmp != NULL){
    free(tmp->commandline);
    struct history *t = tmp;
    tmp = tmp->next;
    free(t);
  }

  return 0;

}


//all of the external functions and built-in commands, like which(), cd(), list(), etc
#include "base_commands.c"

