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
