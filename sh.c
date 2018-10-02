/*

CISC361 Shell

Marc Bolinas
Brian Phillips


 */

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "sh.h"

extern pid_t childpid;



int sh( int argc, char **argv, char **envp )
{
  char *prompt = calloc(PROMPTMAX, sizeof(char));
  char *commandline = calloc(MAX_CANON, sizeof(char));
  char *command, *arg, *currentdir, *previousdir, *pwd, *owd;
  char **args = calloc(MAXARGS, sizeof(char*));
  int uid, i, go = 1;
  struct passwd *password_entry;
  char *homedir;
  struct pathelement *pathlist;

  uid = getuid();
  password_entry = getpwuid(uid);               /* get passwd info */
  homedir = password_entry->pw_dir;/* Home directory to start
				      out with*/
  
  
  currentdir = malloc(sizeof(char) * PATH_MAX);
  previousdir = malloc(sizeof(char) * PATH_MAX);
  strcpy(currentdir, homedir);
  strcpy(previousdir, currentdir);
  chdir(currentdir);


  if((pwd = getcwd(NULL, PATH_MAX+1)) == NULL){
    perror("getcwd");
    exit(2);
  }
  owd = calloc(strlen(pwd) + 1, sizeof(char));
  memcpy(owd, pwd, strlen(pwd));
  prompt[0] = ' '; prompt[1] = '\0';

  /* Put PATH into a linked list */
  pathlist = get_path();


  struct history *histhead = NULL;
  struct history *histtail = NULL;
  int history_length = 10;
  int current_length = 0;

  struct alias_entry *ahead = NULL;
  struct alias_entry *atail = NULL;

  int buffersize = PROMPTMAX;
  char buffer[buffersize];
  char tempbuffer[buffersize];
  strcpy(prompt, "(361)");

  while (go)
    {


      printf("\n%s%s >> ", prompt, currentdir);
     

      /* get command line and process */
      fgets(buffer, buffersize, stdin);
      buffer[(int) strlen(buffer) - 1] = '\0';
      strcpy(tempbuffer, buffer);
      char *tkn;
      tkn = strtok(tempbuffer, " ");
      //the first part of the user input is the command, and technically the first argument


      //if we find an alias, overwrite buffer with the alias's command
      //the reason we need tempbuffer is because strtok() messes up
      //when called twice on the same string
      struct alias_entry *trav = ahead;
      int found_alias = 0;
      while(trav != NULL && found_alias == 0){
	if(strcmp(tkn, trav->key) == 0){
	  strcpy(buffer, trav->command);
	  printf("Executing [%s]\n", buffer);
	  found_alias = 1;
	}
	trav = trav->next;
      }



      char *token;
      token = strtok(buffer, " ");

      if(token != NULL){
	command = malloc(sizeof(char) * (int) strlen(token) + 1);
	args[0] = malloc(sizeof(char) * (int) strlen(token) + 1);
	strcpy(command, token);
	strcpy(args[0], token);
      }
      else{
	command = malloc(0);//if the user doesn't input anything, just malloc() 0 bytes
      }
      token = strtok(NULL, " ");
      

      //because programs assume argv[0] is the program name itself, args[0]
      //cannot be an actual argument, just the program name
      for(i = 1; token != NULL; token = strtok(NULL, " ")){
	args[i] = malloc(sizeof(char) * (int) strlen(token) + 1);
	strcpy(args[i], token);
	i++;
      }
      
      if((int) strlen(command) != 0){
	if(current_length == 0){
	  histhead = malloc(sizeof(struct history));
	  histhead->commandline = malloc(sizeof(char) * strlen(buffer) + 1);
	  strcpy(histhead->commandline, buffer);
	  histtail = histhead;
	  histhead->next = NULL;
	  histhead->prev = NULL;
	  current_length++;
	}
	else{
	  struct history *tmp;
	  tmp = malloc(sizeof(struct history));
	  tmp->commandline = malloc(sizeof(char) * strlen(buffer) + 1);
	  strcpy(tmp->commandline, buffer);
	  tmp->next = histhead;
	  tmp->prev = NULL;
	  histhead->prev = tmp;
	  histhead = tmp;
	  current_length++;
	}
      }
      free(token);
      
      //check for each builtin command
      //some commands are separate functions because they're long
      if((int) strlen(command) == 0){
	//if the user didn't input anything, don't do anything
      }
      else if(strcmp(command, "exit") == 0){
	go = 0;
	printf("Closing shell...\n\n\n");
      }
      else if(strcmp(command, "cd") == 0){
	printf("Executing built-in command cd\n");
	cd(command, args, homedir, currentdir, previousdir);
      }
      else if(strcmp(command, "pwd") == 0){
	printf("Executing built-in command pwd\n");
	char *tmp;
	tmp = getcwd(NULL, 0);
	printf("[%s]\n", tmp);
	free(tmp);
      }
      else if(strcmp(command, "list") == 0){
	printf("Executing built-in command list\n");
	list(command, args, currentdir);
      }
      else if(strcmp(command, "prompt") == 0){
	printf("Executing built-in command prompt\n");
	if(args[1] == NULL){
	  printf("Enter prompt prefix: ");
	  char tmp[PROMPTMAX];
	  fgets(tmp, PROMPTMAX, stdin);
	  tmp[(int) strlen(tmp) - 1] = '\0';
	  strcpy(prompt, tmp);
	}
	else{
	  strcpy(prompt, args[1]);
	}
      }
      else if(strcmp(command, "pid") == 0){
	printf("Executing built-in command pid\n");
	printf("PID: %d\n", getpid());
      }
      else if(strcmp(command, "kill") == 0){
	printf("Executing built-in command kill\n");
	killsig(command, args);
      }
      else if(strcmp(command, "which") == 0){
	printf("Executing built-in command which\n");
	if(args[1] != NULL){
	  for(int j = 1; args[j] != NULL; j++){
	    char *tmp = which(args[j], pathlist);
	    if(tmp == NULL){
	      perror("Command not found");
	    }
	    else{
	      printf("%s\n", tmp);
	      free(tmp);
	    }
	  }
	}
	else{
	  printf("Usage for which: which [command1] ...\n");
	}
      }
      else if(strcmp(command, "where") == 0){
	printf("Executing built-in command where\n");
	if(args[1] != NULL){
	  for(int j = 1; args[j] != NULL; j++){
	    where(args[j], pathlist);
	  }
	}
	else{
	  printf("Usage for where: where [command1] ...\n");
	}
      }
      else if(strcmp(command, "history") == 0){
	printf("Executing built-in command history\n");
	struct history *tmp = histhead;
	int i = 0;
	if(args[1] != NULL){
	  history_length = (int) atoi(args[1]);
	}
	while(tmp != NULL && i < history_length){
	  printf("%s\n", tmp->commandline);
	  tmp = tmp->next;
	  i++;
	}
      }
      else if(strcmp(command, "printenv") == 0){
	printf("Executing built-in command printenv\n");
	printenv(args, envp);
      }
      else if(strcmp(command, "setenv") == 0){
	printf("Executing built-in command setenv\n");
	if(args[1] == NULL){//no args, print environment
	  printenv(args, envp);
	}
	else if(args[2] == NULL){//one arg, it's a new empty environment variable
	  setenv(args[1], "", 1);
	}
	else if(args[3] == NULL){//two args, second arg is value of environment variable
	  setenv(args[1], args[2], 1);
	  if(strcmp(args[1], "HOME") == 0){
	    homedir = getenv("HOME");
	  }
	  else if(strcmp(args[1], "PATH") == 0){
	    pathlist = get_path();
	  }
	}else{
	  printf("Usage for setenv: setenv [VARIABLE] [value]\n");
	}

      }
      else if(strcmp(command, "alias") == 0){
	printf("Executing built-in command alias\n");
	if(args[1] == NULL){
	  struct alias_entry *tmp;
	  tmp = ahead;
	  while(tmp != NULL){
	    printf("alias %s = %s\n", tmp->key, tmp->command);
	    tmp = tmp->next;
	  }
	}
	else if(args[2] == NULL){
	  printf("Usage for alias: alias [shortcut] [full command]");
	}
	else{
	  struct alias_entry *alias;
	  alias = malloc(sizeof(struct alias_entry));
	  alias->key = malloc(sizeof(char) * ABUFFER + 1);
	  alias->command = malloc(sizeof(char) * buffersize + 1);
	  alias->next = NULL;
	  alias->prev = NULL;
	  strcpy(alias->key, args[1]);
	  int sum = 0;
	  for(int i = 2; args[i] != NULL; i++){
	    sum = sum + strlen(args[i]) + 1;
	  }
	  char tmp[sum];
	  strcpy(tmp, args[2]);
	  for(int i = 3; args[i] != NULL; i++){
	    strcat(tmp, " ");
	    strcat(tmp, args[i]);
	  }
	  strcpy(alias->command, tmp);
	  if(ahead == NULL){
	    alias->next = NULL;
	    alias->prev = NULL;
	    ahead = alias;
	    atail = alias;
	  }
	  else{
	    atail->next = alias;
	    alias->prev = atail;
	    atail = alias;
	  }
	  
	}
      }
      else{//if it's not a builtin command, it's either an external command or not valid
	execute_command(command, args, envp, pathlist);

      }
      //printf("command end of loop: %s\n", command);

      command = NULL;
      free(command);
      for(int j = 0; j <= i; j++){
       	args[j] = NULL;
	free(args[j]);
      }
    }

  free(prompt);
  free(commandline);
  free(args);
  //free(homedir);
  free(currentdir);
  free(owd);
  free(pwd);

  struct history *tmp = histhead;
  while(tmp->next != NULL){
    free(tmp->commandline);
    tmp = tmp->next;
    free(tmp->prev);
  }
  free(tmp->commandline);
  free(tmp);

  return 0;
}



char *which(char *command, struct pathelement *pathlist )
{
  struct pathelement *p = pathlist;
  
  while(p){
    int size = (int) strlen(p->element) + (int) strlen(command) + 1;
    char *tmp = malloc(size * sizeof(char) + 1);
    strcpy(tmp, p->element);
    strcat(tmp, "/");
    strcat(tmp, command);

    if(access(tmp, X_OK) == 0){
      return tmp;
    }
    p = p->next;
  }
  return NULL;
}

char *where(char *command, struct pathelement *pathlist )
{
  int found = 0;
  struct pathelement *p = pathlist;
  while(p){
    int size = (int) strlen(p->element) + (int) strlen(command) + 1;
    char *tmp = malloc(size * sizeof(char) + 1);
    strcpy(tmp, p->element);
    strcat(tmp, "/");
    strcat(tmp, command);
    
    if(access(tmp, X_OK) == 0){
      printf("%s\n",tmp);
      found = 1;
    }
    p = p->next;
    free(tmp);
  }

  if(found == 0){
    printf("%s not found\n", command);
  }


  return NULL;
  /* similarly loop through finding all locations of command */
}

//changes directory, outputting new directory to currentdir and chdir()
//returns 0 on success (or incorrect command usage), -1 on fail
int cd(char *command, char **args, char *homedir, char *currentdir, char *previousdir){
  if(args[2] != NULL){
    printf("Usage for cd: cd [directory]\n");
    return 0;
  }
  if(args[1] != NULL && strcmp(args[1], "-") == 0){
    char *tmpdir;
    tmpdir = malloc((sizeof(char) * strlen(currentdir)) + 1);
    strcpy(tmpdir, currentdir);
    strcpy(currentdir, previousdir);
    strcpy(previousdir, tmpdir);
    chdir(previousdir);
    free(tmpdir);
  }
  else if(args[1] != NULL){
    char path_resolved[PATH_MAX];
    if(realpath(args[1], path_resolved) == NULL){
      perror("Directory not found");
      return -1;
    }
    else{
      if(chdir(path_resolved) == 0){
	strcpy(previousdir, currentdir);
	strcpy(currentdir, path_resolved);
      }
      else{
	perror("Could not change into specified directory");
	return -1;
      }
    }
  }
  else{
    strcpy(previousdir, currentdir);
    strcpy(currentdir, homedir);
    chdir(homedir);

    /*
    char path_resolved[PATH_MAX];
    if(realpath("..", path_resolved) == NULL){
      perror("Directory not found");
      return -1;
    }
    else{
      if(chdir(path_resolved) == 0){
	strcpy(currentdir, path_resolved);
      }
      else{
	perror("Could not change into parent directory");
	return -1;
      }
      }*/
  }
  return 0;
}

//lists everything in each specified folder, or in current directory if none specified
//returns 0 on success, -1 on fail
int list (char *command, char **args, char *currentdir)
{
  if(args[1] == NULL){
    DIR *p_dir = opendir(".");
    if(p_dir == NULL){
      perror("Error opening directory");
      return -1;
    }
    else{
      struct dirent *tmp;
      printf("[ %s ]\n", currentdir);
      while((tmp = readdir(p_dir)) != NULL){
	if(strstr(tmp->d_name, ".") != tmp->d_name){
	  printf("%s\n", tmp->d_name);
	}
      }
    }
    closedir(p_dir);
  }
  else{
    for(int i = 1; args[i] != NULL; i++){
      DIR *p_dir = opendir(args[i]);
      if(p_dir == NULL){
	perror("Error opening directory");
	return -1;
      }
      else{
	struct dirent *tmp;
	printf("[ %s ]\n", args[i]);
	while((tmp = readdir(p_dir)) != NULL){
	  if(strstr(tmp->d_name, ".") != tmp->d_name){
	    printf("%s\n", tmp->d_name);
	  }
	}
      }
      closedir(p_dir);
    }
  }
  return 0;
}

//executes external command, either specified by PATH or if command is a directory
int execute_command(char *command, char **args, char **envp, struct pathelement  *pathlist){

  if(strstr(command, "/") == command || strstr(command, ".") == command){
    //command is either an absolute path or relative path                                     

    char path_resolved[PATH_MAX];
    if(realpath(command, path_resolved) == NULL){

      perror("Executable not found");
      return -1;
    }
    else{

      if(access(path_resolved, X_OK) == 0){
	childpid = fork();

	if(childpid < 0){
	  perror("Error forking");
	  exit(1);
	}
	else if(childpid == 0){
	  pid_t mypid = getpid();
	  //printf("child pid: %d\n", mypid);
	  printf("Executing [%s]\n", path_resolved);
	  if(execve(path_resolved, args, envp) == -1){
	    perror("Could not execute program");
	    kill(mypid, SIGKILL);
	    return -1;
	  }
	}
	else{
	  waitpid(childpid, NULL, 0);
	}
	childpid = 0;
      }
      else{
	perror("Could not access executable");
	return -1;
      }
    }

  }
  else{
    char *tmp = which(command, pathlist);
    if(tmp != NULL){

      childpid = fork();

      if(childpid < 0){
	perror("Error when forking");
	exit(1);
      }
      else if(childpid == 0){
	pid_t mypid = getpid();
	printf("Executing [%s]\n", tmp);
	if(execve(tmp, args, envp) == -1){
	  perror("Killing child process");
	  free(tmp);
	  kill(mypid, SIGKILL);
	  return -1;
	}
      }
      else{
	waitpid(childpid, NULL, 0);
      }
      childpid = 0;

    }
    else{
      printf("%s: Command not found", command);
    }
    free(tmp);
  }
  return 0;
}

int killsig(char *command, char **args){
  if(args[1] == NULL){
    printf("Usage for kill: kill [-signal_number] [pid]\n(signal_number is optional)\n");
    return -1;
  }
  else if(args[2] == NULL){//only one argument, stored in args[1], a pid
    int pid = atoi(args[1]);

    if(pid == getpid()){
      //if we're trying to kill our own shell process, don't                        
      printf("Cannot kill own process\nUse: kill -1 [this pid]\nto force kill\n");
      return -1;
    }

    childpid = pid;
    if(kill(childpid, SIGTERM) == -1){
      perror("Error killing process");
      return -1;
    }
    childpid = 0;
  }
  else{
    int signal = atoi(args[1]+1);

    if(signal > 31){
      signal = 0;
    }
    char **p_args = &args[1]+1;
    int pid = atoi(args[2]);

    childpid = pid;
    if(kill(childpid, signal) == -1){
      perror("Error killing process");
      return -1;
    }
    childpid = 0;
  }

  return 0;
}

void printenv(char **args, char **envp){
  if(args[1] == NULL){
    int i = 0;
    while(envp[i] != NULL){
      printf("%s\n", envp[i]);
      i++;
    }
  }
  else if(args[2] == NULL){
    char *str = getenv(args[1]);
    if(str != NULL){
      printf("[%s]: %s\n", args[1], str);
    }
  }
  else{
    printf("Usage for printenv: printenv [arg1] [arg2]\n");
  }
}
