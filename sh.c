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

extern pid_t c_pid;

int sh( int argc, char **argv, char **envp )
{
  char *prompt = calloc(PROMPTMAX, sizeof(char));
  char *commandline = calloc(MAX_CANON, sizeof(char));
  char *command, *arg, *currentdir, *p, *pwd, *owd;
  char **args = calloc(MAXARGS, sizeof(char*));
  int uid, i, status, argsct, go = 1;
  struct passwd *password_entry;
  char *homedir;
  struct pathelement *pathlist;

  uid = getuid();
  password_entry = getpwuid(uid);               /* get passwd info */
  homedir = password_entry->pw_dir;/* Home directory to start
				      out with*/
  
  
  currentdir = malloc(sizeof(char) * PATH_MAX);
  strcpy(currentdir, homedir);
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


  int buffersize = PROMPTMAX;
  char buffer[buffersize];

  strcpy(prompt, "(361)");

  while (go)
    {
      printf("\n%s%s >> ", prompt, currentdir);
     

      /* get command line and process */
      fgets(buffer, buffersize, stdin);
      buffer[(int) strlen(buffer) - 1] = '\0';

      char *token;
      token = strtok(buffer, " ");
      //the first part of the user input is the command, and technically the first argument
      if(token != NULL){
	command = malloc(sizeof(char) * (int) strlen(token));
	args[0] = malloc(sizeof(char) * (int) strlen(token));
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
	args[i] = malloc(sizeof(char) * (int) strlen(token));
	strcpy(args[i], token);
	i++;
      }
      
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
	cd(command, args, homedir, currentdir);
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
	printf("PID: %d\n", getpid());
      }
      else if(strcmp(command, "kill") == 0){
	killsig(command, args);
      }
      else{//if it's not a builtin command, it's either an external command or not valid
	execute_command(command, args, envp, pathlist);
      }



      command = NULL;
      free(command);
      for(int j = 0; j < i; j++){
	args[j] = NULL;
	free(args[j]);
      }
    }

  free(prompt);
  free(commandline);
  free(args);
  free(currentdir);
  free(owd);
  free(pwd);
  return 0;
}



char *which(char *command, struct pathelement *pathlist )
{
  struct pathelement *p = pathlist;
  
  while(p){
    int size = (int) strlen(p->element) + (int) strlen(command) + 1;
    char *tmp = malloc(size * sizeof(char));
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
  return NULL;
  /* similarly loop through finding all locations of command */
}

//changes directory, outputting new directory to currentdir and chdir()
//returns 0 on success (or incorrect command usage), -1 on fail
int cd(char *command, char **args, char *homedir, char *currentdir){
  if(args[2] != NULL){
    printf("Usage for cd: cd [directory]\n");
    return 0;
  }
  if(args[1] != NULL && strcmp(args[1], "-") == 0){
    strcpy(currentdir, homedir);
    chdir(homedir);
  }
  else if(args[1] != NULL){
    char path_resolved[PATH_MAX];
    if(realpath(args[1], path_resolved) == NULL){
      perror("Directory not found");
      return -1;
    }
    else{
      //strcpy(currentdir, path_resolved);
      if(chdir(currentdir) == -1){
	perror("Could not change into specified directory");
	return -1;
      }
      strcpy(currentdir, path_resolved);
    }
  }
  else{
    char path_resolved[PATH_MAX];
    if(realpath("..", path_resolved) == NULL){
      perror("Directory not found");
      return -1;
    }
    else{
      //strcpy(currentdir, path_resolved);
      if(chdir(currentdir) == -1){
	perror("Could not change into parent directory");
	return -1;
      }
      strcpy(currentdir, path_resolved);
    }
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
	pid_t pid;
	pid = fork();

	if(pid < 0){
	  perror("Error forking");
	  exit(1);
	}
	else if(pid == 0){
	  pid_t mypid = getpid();
	  printf("child pid: %d\n", mypid);
	  printf("Executing [%s]\n", path_resolved);
	  if(execve(path_resolved, args, envp) == -1){
	    perror("Could not execute program");
	    kill(mypid, SIGKILL);
	    return -1;
	  }
	}
	else{
	  waitpid(pid, NULL, 0);
	}
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

      pid_t pid;
      pid = fork();

      if(pid < 0){
	perror("Error when forking");
	exit(1);
      }
      else if(pid == 0){
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
	waitpid(pid, NULL, 0);
      }


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
    if(kill(pid, SIGTERM) == -1){
      perror("Error killing process");
      return -1;
    }
  }
  else{
    int signal = atoi(args[0]+1);
    if(signal > 31){
      signal = 0;
    }
    char **p_args = args+1;
    for(int i = 1; *p_args != NULL; i++){
      int pid = atoi(args[i]);
      if(kill(pid, signal) == -1){
	perror("Error killing process");
      }
      p_args++;
    }
  }

  return 0;
}
