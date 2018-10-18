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
     //printf("old: %ld, new: %lld\n", old, path.st_size);
     time(&start);
     stat(file, &path);
     if((long)path.st_size != old){
       printf("\nBEEP! You got mail in %s at time %s\n", file, ctime(&start));
       fflush(stdout);
       old = (long)path.st_size;
     }
     sleep(3);
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
  int background = 0;
  if(args[argcount - 1] != NULL && strcmp(args[argcount - 1], "&") == 0){
    printf("found &\n");
    background = 1;
    args[argcount - 1] = NULL;
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
	  //printf("Executing [%s]\n", path_resolved);
	  if(execve(path_resolved, args, envp) == -1){
	    perror("Could not execute program");
	    kill(mypid, SIGKILL);
	    return -1;
	  }
	}
	else{
	  if(background == 1){
	    struct children *tmp;
	    tmp = malloc(sizeof(struct children));
	    tmp->pid = childpid;
	    tmp->next = NULL;
	    if(childhead == NULL){
	      childhead = tmp;
	    }
	    else{
	      tmp->next = childhead;
	      childhead = tmp;
	    }
	    
	  }
	  else{
	    waitpid(childpid, NULL, 0);
	    childpid = 0;
	  }
	}
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
	//printf("Executing [%s]\n", tmp);
	if(execve(tmp, args, envp) == -1){
	  perror("Killing child process");
	  free(tmp);
	  kill(mypid, SIGKILL);
	  return -1;
	}
      }
      else{
	if(background == 1){
	  struct children *tmp;
	  tmp = malloc(sizeof(struct children));
	  tmp->pid = childpid;
	  tmp->next = NULL;
	  if(childhead == NULL){
	    childhead = tmp;
	  }
	  else{
	    tmp->next = childhead;
	    childhead = tmp;
	  }
	  childpid = 0;
	}
	else{
	  waitpid(childpid, NULL, 0);
	  childpid = 0;
	}
      }

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
