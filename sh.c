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
  char *command, *arg, *commandpath, *p, *pwd, *owd;
  char **args = calloc(MAXARGS, sizeof(char*));
  int uid, i, status, argsct, go = 1;
  struct passwd *password_entry;
  char *homedir;
  struct pathelement *pathlist;

  uid = getuid();
  password_entry = getpwuid(uid);               /* get passwd info */
  homedir = password_entry->pw_dir;/* Home directory to start
				      out with*/

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

  // struct pathelement *tmp;
  //tmp = pathlist;
  //while(tmp->next != NULL){
  //printf("directory: %s\n", tmp->element);
  //tmp = tmp->next;
  //}



  while ( go )
    {

      //printf("value of args[0] = %s\n", args[0]);
      //printf("value of args[1] = %s\n", args[1]);



      /* print your prompt */
      printf("\n(361)%s >> ", homedir);
      /* get command line and process */
      fgets(buffer, buffersize, stdin);

      buffer[(int) strlen(buffer) - 1] = '\0';

      char *token;
      token = strtok(buffer, " ");
      
      for(i = 0; token != NULL; token = strtok(NULL, " ")){
	args[i] = malloc(sizeof(char) * (int) strlen(token));
	strcpy(args[i], token);
	i++;
      }

      if(args[0] == NULL)
	args[0] = malloc(0);

      for(int j = 0; j < i; j++){
	//printf("argument %d: %s\n", j, args[j]);
      }
      
      /* check for each built in command and implement */

      if(strcmp(args[0], "exit") == 0){
	go = 0;
	printf("Closing shell...\n\n\n");
      }
      else if(strcmp(args[0], "fork") == 0){
	
      }
      else{
	//else program to exec
	
	char *tmp = which(args[0], pathlist);
	
	//execve(tmp, &args[1], envp);

	if(tmp != NULL){
	  printf("Command found in: %s\n", tmp);
	  
	  pid_t pid;
	  pid = fork();
	  
	  if(pid < 0){
	    perror("Error when forking");
	  }
	  else if(pid == 0){
	    pid_t mypid = getpid();
	    printf("hello from child %d\n", mypid);

	    //execve(tmp, &args[1], envp);
	    printf("%s\n", argv[1]);
	    if(execve(tmp, &argv[1], envp) == -1){
	      kill(mypid, SIGKILL);
	    }

	    
	    //printf("execve returned: %d\n", i);
	    
	    //sleep(1);

	    
	    //printf("rip child\n");
	    //kill(mypid, SIGKILL);
	  }
	  else{
	    waitpid(pid, NULL, 0);
	    //pid_t mypid = getpid();
	    //printf("hello from parent %d\n", mypid);

	    printf("parent reactivated\n");

	  }


	}
	else if((int) strlen(args[0]) != 0){
	  printf("%s: Command not found\n", args[0]);

	}
      }



      /*  else  program to exec */
      //{
	/* find it */
	/* do fork(), execve() and waitpid() */

	//else
	//fprintf(stderr, "%s: Command not found.\n", args[0]);
	//}

      
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
    //char tmp[size];
    char *tmp = malloc(size * sizeof(char));
    strcpy(tmp, p->element);
    strcat(tmp, "/");
    strcat(tmp, command);

    if(access(tmp, F_OK) == 0){
      //printf("returning %s\n", tmp);
      return tmp;
    }
    p = p->next;
  }
  
  

  return NULL;
  /* loop through pathlist until finding command and return it.  Return
     NULL when not found. */

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
