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

  while ( go )
    {
      /* print your prompt */
      printf("361shell>> ");
      /* get command line and process */
      fgets(buffer, buffersize, stdin);
      buffer[(int) strlen(buffer) - 1] = '\0';

      
      char *token;
      char *cmmd = malloc(buffersize * sizeof(char));


      token = strtok(buffer, " ");
      if(token != NULL){
	strcpy(cmmd, token);
      }
      
      //printf("Command entered: %s\n", cmmd);

      
      i = 0;
      while(token != NULL){
	//printf("%s\n", token);

	args[i] = malloc(sizeof(char) * (int) strlen(token));
	strcpy(args[i], token);
	i++;
	token = strtok(NULL, " ");
      }

      command = malloc(sizeof(char) * (int) strlen(args[0]));
      strcpy(command, args[0]);
      printf("command: %s\n", command);
      for(int j = 1; j < i; j++){
	printf("argument %d: %s\n", j, args[j]);
      }


      /* check for each built in command and implement */

      /*  else  program to exec */
      //{
	/* find it */
	/* do fork(), execve() and waitpid() */

	//else
	//fprintf(stderr, "%s: Command not found.\n", args[0]);
	//}



      free(command);
      
    }
  return 0;
} /* sh() */

char *which(char *command, struct pathelement *pathlist )
{
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
  /* see man page for opendir() and readdir() and print out filenames for
     the directory passed */
} /* list() */
