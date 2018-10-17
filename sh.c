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
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int argcount = 0;

int sh( int argc, char **argv, char **envp )
{
  char *prompt = calloc(PROMPTMAX, sizeof(char));
  char *commandline = calloc(MAX_CANON, sizeof(char));
  char *command, *arg, *currentdir, *previousdir, *pwd, *owd;
  char **args = calloc(MAXARGS, sizeof(char*));
  int uid, i, go = 1, watchthread = 0, mailthread = 0;
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
     
      free(token);

      argcount = i;
      printf("argcount = %d\n", argcount);
      
      //check for each builtin command
      //some commands are separate functions because they're long
      if(input_error == 1){
	//if the user didn't input anything or didn't input correctly, don't do anything
      }
      else if(strcmp(command, "exit") == 0){
	printf("Executing built-in command exit\n");
	go = 0;
	printf("Closing shell...\n\n\n");
      }
      else if(strcmp(command, "cd") == 0){
	printf("Executing built-in command cd\n");
	if(expanded) args[2] = NULL;//if we expanded a * or ?, use the first result, args[1]
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
      //watchmail command
      else if(strcmp(command, "watchmail") == 0){
	printf("Watch mail initiated\n");

	if(args[2] == NULL){
	  //two arguemnts, meaning start watching one file
	  struct stat buff;
	  int exists = stat(args[1], &buff);
	  printf("exists? %d\n", exists);
	  if(exists == 0){
	    pthread_t mail_t;

	    char* filepath = (char *)malloc(strlen(args[1]));
	    strcpy(filepath, args[1]);
	    printf("%s\n", filepath);
	    pthread_create(&mail_t, NULL, watchmail, (void *)filepath);
	    
	    if(mailthread == 0 || watchmailhead == NULL){
	      mailthread = 1;
	      watchmailhead = malloc(sizeof(struct maillist));
	      watchmailhead->str = malloc(sizeof(strlen(filepath)));
	      strcpy(watchmailhead->str, filepath);
	      watchmailhead->id = mail_t;
	    }else{
	      struct maillist *tmp = watchmailhead;
	      while(tmp->next != NULL){
		tmp = tmp->next;
	      }
	      tmp->next = malloc(sizeof(struct maillist));
	      tmp->next->str = malloc(sizeof(strlen(filepath)));
	      strcpy(tmp->next->str, filepath);
	      tmp->next->id = mail_t;
	    }
	  }
	}else if(args[2] != NULL){
	  //Remove head from watchlist
	  if(strcmp(watchmailhead->str, args[1]) == 0){
	    struct maillist *tmp = watchmailhead;
	    watchmailhead = watchmailhead->next;
	    pthread_cancel(tmp->id);
	    int pj = pthread_join(tmp->id, NULL);
	    printf("joined? %d\n", pj);
	  }else{
	    //Remove another node from watchlist
	    struct maillist *tmp2 = watchmailhead;
	    while(strcmp(tmp2->next->str, args[1]) != 0){
	      tmp2 = tmp2->next;
	    }
	    if(strcmp(tmp2->next->str, args[1]) == 0){
	      pthread_cancel(tmp2->next->id);
	      printf("joining thread\n");
	      int j = pthread_join(tmp2->id, NULL);
	      printf("joined? %d\n", j);
	      tmp2->next = tmp2->next->next;
	    }else{
	      printf("File not being watched\n");
	    }
	  }
	}
      }
	  
	  
      

      
      //watchuser command
      else if(strcmp(command, "watchuser") == 0){
	printf("Executing built-in command watchuser\n");

	if(watchthread == 0){
	  printf("Starting watchuser thread...\n");
	  watchthread = 1;
	  pthread_t watchuser_t;
	  pthread_create(&watchuser_t, NULL, watchuser, args[1]);
	}

	if(args[1] == NULL){
	  printf("Usage for watchuser: watchuser [user] [off (optional)]\n");
	}
	else{
	  pthread_mutex_lock(&mutex);
	  if(args[2] != NULL && strcmp(args[2], "off") == 0){//remove from linked list of users to watch
	    struct strlist *tmp = watchuserhead;

	    while(tmp != NULL){
	      if(strcmp(tmp->str, args[1]) == 0){
		if(tmp->prev == NULL){//deleting the head of the list
		  printf("Deleting head %s\n", tmp->str);
		  if(tmp->next == NULL){
		    watchuserhead = NULL;
		  }
		  else{
		    watchuserhead = tmp->next;
		    watchuserhead->prev = NULL;
		  }
		  free(tmp->str);
		  free(tmp);
		  tmp = watchuserhead;
		}
		else{
		  printf("Deleting %s\n", tmp->str);
		  if(tmp->next == NULL){
		    tmp->prev->next = NULL;
		  }
		  else{
		    tmp->prev->next = tmp->next;
		  }
		  free(tmp->str);
		  free(tmp);
		  tmp = watchuserhead;
		}
	      }
	      else{
		tmp = tmp->next;
	      }
	    }

	    printf("Watchuser list is now..\n");
	    tmp = watchuserhead;
	    while(tmp != NULL){
	      printf("User: %s\n", tmp->str);
	      tmp = tmp->next;
	    }

	  }
	  else{//add to linked list of users to watch

	    //TO-DO: add mutex locks so that watchuser_t doesn't write tmp->status

	    if(watchuserhead == NULL){
	      printf("Adding new head: %s\n", args[1]);
	      struct strlist *tmp;
	      tmp = malloc(sizeof(struct strlist));
	      tmp->next = NULL;
	      tmp->prev = NULL;
	      tmp->status = 0;
	      tmp->str = malloc((sizeof(char) * strlen(args[1])) + 1);
	      strcpy(tmp->str, args[1]);
	      watchuserhead = tmp;
	    }
	    else{
	      printf("Adding to list: %s\n", args[1]);
	      struct strlist *tmp;
	      tmp = malloc(sizeof(struct strlist));
	      tmp->str = malloc((sizeof(char) * strlen(args[1])) + 1);
	      strcpy(tmp->str, args[1]);
	      tmp->next = watchuserhead;
	      tmp->prev = NULL;
	      tmp->status = 0;
	      watchuserhead->prev = tmp;
	      watchuserhead = tmp;
	    }
	  }
	  pthread_mutex_unlock(&mutex);
	}
      }




      
      else if(strcmp(command, "alias") == 0){
	printf("Executing built-in command alias\n");
	if(args[1] == NULL){//no args passed, then print all aliases
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
	else{//otherwise add commandline as a new alias
	  struct alias_entry *alias;
	  alias = malloc(sizeof(struct alias_entry));
	  alias->key = malloc(sizeof(char) * ABUFFER + 1);
	  alias->command = malloc(sizeof(char) * buffersize + 1);
	  alias->next = NULL;
	  alias->prev = NULL;
	  strcpy(alias->key, args[1]);
	  int sum = 0;//we're saving all the args as a big command, sum is the size of them all
	  //including spaces
	  for(int i = 2; args[i] != NULL; i++){//sum arg sizes
	    sum = sum + strlen(args[i]) + 1;
	  }
	  char tmp[sum];
	  strcpy(tmp, args[2]);
	  for(int i = 3; args[i] != NULL; i++){//concatenate into one big string
	    strcat(tmp, " ");
	    strcat(tmp, args[i]);
	  }
	  strcpy(alias->command, tmp);
	  if(ahead == NULL){//then add the alias to the list
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

//watchmail
 void *watchmail(void *arg){
   char* file = (char*)arg;
   struct stat path;
   
   stat(file, &path);
   long old = (long)path.st_size;
   time_t start;
   while(1){
     time(&start);
     stat(file, &path);
     if((long)path.st_size != old){
       printf("\nBEEP! You got mail in %s at time %s\n", file, ctime(&start));
       fflush(stdout);
       old = (long)path.st_size;
     }
     sleep(1);
   }
 }

   
 
//Implement watchuser
void *watchuser(void *arg){
  
  struct utmpx *up;

  while(1){
    setutxent();
    
    while((up = getutxent())){
      if(up->ut_type == USER_PROCESS){

	pthread_mutex_lock(&mutex);
	struct strlist *tmp;
	tmp = watchuserhead;
	while(tmp != NULL){
	  if((tmp->status == 0) && strcmp(tmp->str, up->ut_user) == 0){
	    tmp->status = 1;
	    printf("\n%s has logged on [%s] from [%s]\n", up->ut_user, up->ut_line, up->ut_host);
	  }
	  tmp = tmp->next;
	}
	pthread_mutex_unlock(&mutex);
      }
    }





    //printf("\n");
    sleep(10);
  }
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
  if(args[1] != NULL && strcmp(args[1], "-") == 0){//a '-' means to go to previous dir
    chdir(previousdir);
    char *tmpdir;
    tmpdir = malloc((sizeof(char) * strlen(currentdir)) + 1);
    strcpy(tmpdir, currentdir);
    strcpy(currentdir, previousdir);
    strcpy(previousdir, tmpdir);
    free(tmpdir);
  }
  else if(args[1] != NULL){
    char path_resolved[PATH_MAX];
    if(realpath(args[1], path_resolved) == NULL){//converts a relative path to an absolute path
      perror("Directory not found");
      return -1;
    }
    else{
      if(chdir(path_resolved) == 0){//path resolved is the absolute path
	strcpy(previousdir, currentdir);
	strcpy(currentdir, path_resolved);
      }
      else{
	perror("Could not change into specified directory");
	return -1;
      }
    }
  }
  else{//otherwise go back to homedir
    strcpy(previousdir, currentdir);
    strcpy(currentdir, homedir);
    chdir(homedir);
  }
  return 0;
}

//lists everything in each specified folder, or in current directory if none specified
//returns 0 on success, -1 on fail
int list (char *command, char **args, char *currentdir)
{
  if(args[1] == NULL){//no args passed, open directory currently in
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
  else{//otherwise open every arg
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

  if(strcmp(args[argcount - 1], "&") == 0){
    printf("found &\n");
  }

  if(strstr(command, "/") == command || strstr(command, ".") == command){
    //command is either an absolute path or relative path                                     

    char path_resolved[PATH_MAX];
    if(realpath(command, path_resolved) == NULL){//converts relative path to absolute path

      perror("Executable not found");
      return -1;
    }
    else{

      if(access(path_resolved, X_OK) == 0){//if we can exec the absolute path, fork() then exec()
	childpid = fork();

	if(childpid < 0){
	  perror("Error forking");
	  exit(1);
	}
	else if(childpid == 0){
	  pid_t mypid = getpid();
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
  else{//otherwise the command is listed in pathlist
    char *tmp = which(command, pathlist);
    if(tmp != NULL){

      childpid = fork();
      //found command, then fork() and exec() like usual
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
    else{//if it's not in pathlist then the command doesn't exist
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
  else{//parse the signal from args[1]
    int signal = atoi(args[1]+1);//+1 removes the '-' in front

    if(signal > 31){
      signal = 0;//signal should be between 0 <= 31
    }
    char **p_args = &args[1]+1;
    int pid = atoi(args[2]);
    
    //childpid is external global var from main.c, used for signal handling
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
  if(args[1] == NULL){//no args, print out all envp
    int i = 0;
    while(envp[i] != NULL){
      printf("%s\n", envp[i]);
      i++;
    }
  }
  else if(args[2] == NULL){//only args[1], print out it's args
    char *str = getenv(args[1]);
    if(str != NULL){
      printf("[%s]: %s\n", args[1], str);
    }
  }
  else{
    printf("Usage for printenv: printenv [arg1] [arg2]\n");
  }
}
