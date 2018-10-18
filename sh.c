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
  childhead = malloc(sizeof(struct children));
  childhead->pid = 0;
  childhead->next = NULL;

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

  //both history and alias_entry are doubly linked lists
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
	  printf("Executing alias [%s]\n", buffer);
	  found_alias = 1;
	}
	trav = trav->next;
      }



      char *token;
      token = strtok(buffer, " ");
      //set command (and arg[0] also stores command)
      if(token != NULL){
	command = malloc(sizeof(char) * (int) strlen(token) + 1);
	args[0] = malloc(sizeof(char) * (int) strlen(token) + 1);
	strcpy(command, token);
	strcpy(args[0], token);
	printf("Executing command %s\n", command);
      }
      else{
	input_error = 1;
	command = malloc(0);//if the user doesn't input anything, just malloc() 0 bytes
      }
      token = strtok(NULL, " ");
      


      //args[] assignment
      for(i = 1; token != NULL; token = strtok(NULL, " ")){
	//if an argument the user passed in has * or ?, then expand it
	//but if the command was 'alias', then don't expand it because we're making a shortcut
	//(the user might want their shortcut to purposefully have a * or ?)
	if((strstr(token, "*") != NULL || strstr(token, "?") != NULL) &&
	   strcmp(command, "alias") != 0){
	  
	  expanded = 1;
	  //expand the * or ?
	  wordexp_t *expanded = malloc(sizeof(wordexp_t));
	  wordexp(token, expanded, 0);
	  //for each expanded word, add it to args[] (so long as it was expanded successfully)
	  for(int j = 0; expanded->we_wordv[j] != NULL; j++){
	    if(strstr(expanded->we_wordv[j], "*") != NULL || 
	       strstr(expanded->we_wordv[j], "?") != NULL){
	      printf("No matching files or directories in: %s\n", expanded->we_wordv[j]);
	      input_error = 1;
	    }
	    else{
	      args[i] = malloc((sizeof(char) * (int) strlen(expanded->we_wordv[j])) + 1);
	      strcpy(args[i], expanded->we_wordv[j]);
	      i++;
	    }

	  }
	}
	else{//otherwise we didn't have to expand it, so add it to args like normal
	  args[i] = malloc(sizeof(char) * (int) strlen(token) + 1);
	  strcpy(args[i], token);
	  i++;
	}       
      }
     
      if(args[1] == NULL){
	i = 0;
      }

      free(token);

      argcount = i;
      //printf("argcount = %d\n", argcount);

      int redirect = 0;
      char *redir_type;
      char *redir_dest;

      int save_stdin = dup(STDIN_FILENO);
      int save_stdout = dup(STDOUT_FILENO);
      int save_stderr = dup(STDERR_FILENO);
      int fileid;


      for(int j = 1; j < argcount; j++){
	if(strcmp(args[j], ">") == 0 || strcmp(args[j], ">&") == 0 || strcmp(args[j], ">>") == 0 || strcmp(args[j], ">>&") == 0 || strcmp(args[j], "<") == 0){
	  redirect = j;
	  redir_type = args[j];
	  redir_dest = args[j + 1];
	  //printf("found %s\n", redir_type);
	}
      }

      if(redirect != 0){
	//printf("Setting up redirections..\n");
	struct stat sttmp;

	

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
	  //printf("Redirecting off of '>>'\n");
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
	args[redirect] = NULL;
	args[redirect + 1] = NULL;

      }



      int pipel = -1;
      //printf("argcount = %d\n", argcount);
      for(int j = 0; j < argcount; j++){
	if(args[j] != NULL && strcmp(args[j], "|") == 0){
	  pipel = j;
	}
      }

      if(pipel == 0 || pipel == argcount){
	pipel = 0;
	input_error = 1;
      }

      char **leftargs = calloc(MAXARGS, sizeof(char*));
      char **rightargs = calloc(MAXARGS, sizeof(char*));
      pid_t leftchild = -1, rightchild = -1;
      

      if(pipel != 0 && pipel != -1){
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

	//printf("right command: %s\n", rightargs[0]);

	int ipc[2];
	if(pipe(ipc) != 0){
	  perror("Error when parsing '|', ");
	  exit(0);
	}


	leftchild = fork();
	if(leftchild == 0){
	  args = leftargs;
	  go = 0;
	  close(STDOUT_FILENO);
	  dup(ipc[1]);
	  close(ipc[0]);
	  //printf("left child! %s\n", command);
	  //sleep(7);
	}
	else{
	  rightchild = fork();
	  if(rightchild == 0){
	    args = rightargs;
	    command = NULL;
	    free(command);
	    command = malloc(1 + (sizeof(char) * strlen(args[0])));
	    strcpy(command, args[0]);
	    go = 0;
	    close(STDIN_FILENO);
	    dup(ipc[0]);
	    close(ipc[1]);
	    //printf("right child! %s\n", command);
	    //sleep(3);
	  }
	  else{
	    input_error = 1;
	    //waitpid(leftchild, NULL, 0);
	    
	    close(ipc[0]);
	    close(ipc[1]);

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

      
      



      //      printf("rightarg0: %s\n", rightargs[0]);




      
      //check for each builtin command
      //some commands are separate functions because they're long
      if(input_error == 1){
	//if the user didn't input anything or didn't input correctly, don't do anything
      }
      else if(strcmp(command, "exit") == 0){
	//	printf("Executing built-in command exit\n");
	go = 0;
	printf("Closing shell...\n\n\n");
      }
      else if(strcmp(command, "cd") == 0){
	//	printf("Executing built-in command cd\n");
	if(expanded) args[2] = NULL;//if we expanded a * or ?, use the first result, args[1]
	cd(command, args, homedir, currentdir, previousdir);
      }
      else if(strcmp(command, "pwd") == 0){
	//	printf("Executing built-in command pwd\n");
	char *tmp;
	tmp = getcwd(NULL, 0);
	printf("[%s]\n", tmp);
	free(tmp);
      }
      else if(strcmp(command, "list") == 0){
	//	printf("Executing built-in command list\n");
	list(command, args, currentdir);
      }
      else if(strcmp(command, "prompt") == 0){
	//	printf("Executing built-in command prompt\n");
	#include "prompt.c"
      }
      else if(strcmp(command, "pid") == 0){
	//	printf("Executing built-in command pid\n");
	printf("PID: %d\n", getpid());
      }
      else if(strcmp(command, "kill") == 0){
	//	printf("Executing built-in command kill\n");
	killsig(command, args);
      }
      else if(strcmp(command, "which") == 0){
	//	printf("Executing built-in command which\n");
	#include "which.c"
      }
      else if(strcmp(command, "where") == 0){
	//	printf("Executing built-in command where\n");
	#include "where.c"
      }
      else if(strcmp(command, "history") == 0){
	//	printf("Executing built-in command history\n");
	#include "history.c"
      }
      else if(strcmp(command, "printenv") == 0){
	//	printf("Executing built-in command printenv\n");
	printenv(args, envp);
      }
      else if(strcmp(command, "setenv") == 0){
	//	printf("Executing built-in command setenv\n");
	#include "setenv.c"
      }
      else if(strcmp(command, "watchmail") == 0){
	//	printf("Watch mail initiated\n");
	#include "watchmail.c"
      }
      else if(strcmp(command, "watchuser") == 0){
	//	printf("Executing built-in command watchuser\n");
	#include "watchuser.c"
      }
      else if(strcmp(command, "noclobber") == 0){
	//	printf("Executing built-in command noclobber\n");
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
	//	printf("Executing built-in command alias\n");
	#include "alias.c"
      }
      else{//if it's not a builtin command, it's either an external command or not valid
	//	printf("Ready to execute external command...\n");
	execute_command(command, args, envp, pathlist);

      }

      //replace stdout and stdin if they were modified
      if(sin == 1){
	//printf("fixing in\n");
	sin = 0;
	close(fileid);
	dup2(save_stdin, 0);
      }
      if(sout == 1){
	//printf("fixing out\n");
	sout = 0;
	close(fileid);
	dup2(save_stdout, 1);
      }
      if(serr == 1){
	//printf("fixing err\n");
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


#include "base_commands.c"

