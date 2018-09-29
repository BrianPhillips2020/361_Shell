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

  //char currentdir[PATH_MAX];
  currentdir = malloc(sizeof(char) * PATH_MAX);
  strcpy(currentdir, homedir);
  chdir(currentdir);


  if ( (pwd = getcwd(NULL, PATH_MAX+1)) == NULL )
    {
      perror("getcwd");
      exit(2);
    }
  owd = calloc(strlen(pwd) + 1, sizeof(char));
  memcpy(owd, pwd, strlen(pwd));
  prompt[0] = ' '; prompt[1] = '\0';

  /* Put PATH into a linked list */
  pathlist = get_path();


  int buffersize = 256;
  char buffer[buffersize];

  strcpy(prompt, "(361)");

  while ( go )
    {

      /* print your prompt */
      printf("\n%s%s >> ", prompt, currentdir);
      /* get command line and process */
      fgets(buffer, buffersize, stdin);
      buffer[(int) strlen(buffer) - 1] = '\0';

      char *token;
      token = strtok(buffer, " ");

      if(token != NULL){
	command = malloc(sizeof(char) * (int) strlen(token));
	args[0] = malloc(sizeof(char) * (int) strlen(token));
	strcpy(command, token);
	strcpy(args[0], token);
      }
      else{
	command = malloc(0);
      }
      token = strtok(NULL, " ");
      

      //because programs assume argv[0] is the program name itself, args[0]
      //cannot be an actual argument, just the program name
      for(i = 1; token != NULL; token = strtok(NULL, " ")){
	//printf("allocating args\n");
	args[i] = malloc(sizeof(char) * (int) strlen(token));
	strcpy(args[i], token);
	i++;
      }
      
      /* check for each built in command and implement */
      if((int) strlen(command) == 0){
	//don't do anything
      }
      else if(strcmp(command, "exit") == 0){
	go = 0;
	printf("Closing shell...\n\n\n");
      }
      else if(strcmp(command, "cd") == 0){
	if(args[1] != NULL && strcmp(args[1], "-") == 0){
	  strcpy(currentdir, homedir);
	  chdir(homedir);
	}
	else if(args[1] != NULL){
	  char path_resolved[PATH_MAX];
	  if(realpath(args[1], path_resolved) == NULL){
	    printf("weird error\n");
	  }
	  else{
	    strcpy(currentdir, path_resolved);
	    chdir(currentdir);
	  }
	}
	else{
	  char path_resolved[PATH_MAX];
	  if(realpath("..", path_resolved) == NULL){
	    printf("weird error\n");
	  }
	  else{
	    strcpy(currentdir, path_resolved);
	    chdir(currentdir);
	  }
	}
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
      else{
	//else program to exec

	if(strstr(command, "/") == command || strstr(command, ".") == command){
	  //command is either an absolute path or relative path
	


	  char path_resolved[PATH_MAX];
	  if(realpath(command, path_resolved) == NULL){
	    printf("Executable not found after parsing: %s\n", command);
	  }
	  else{
	    if(access(path_resolved, X_OK) == 0){
	      pid_t pid;
	      pid = fork();
	      
	      if(pid < 0){
		exit(1);
	      }
	      else if(pid == 0){
		pid_t mypid = getpid();
		printf("Executing [%s]\n", path_resolved);
		if(execve(path_resolved, args, envp) == -1){
		  printf("[%s] is a directory, not an executable\n", path_resolved);
		  kill(mypid, SIGKILL);
		}
	      }
	      else{
		waitpid(pid, NULL, 0);
	      }
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
		printf("killing child...\n");
		kill(mypid, SIGKILL);
	      }
	    }
	    else{
	      waitpid(pid, NULL, 0);
	      //printf("parent reactivated\n");
	      
	    }
	    
	    
	  }
	  else{
	    fprintf(stderr, "%s: Command not found\n", command);
	  }
	}
      }




      command = NULL;
      free(command);
      for(int j = 0; j < i; j++){
	args[j] = NULL;
	free(args[j]);
      }
    }
  return 0;
} /* sh() */



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
} /* which() */

char *where(char *command, struct pathelement *pathlist )
{
  return NULL;
  /* similarly loop through finding all locations of command */
} /* where() */

void list ( char *dir )
{

  if(dir == NULL){
    printf("hello!\n");
  }
  /* see man page for opendir() and readdir() and print out filenames for
     the directory passed */
} /* list() */
