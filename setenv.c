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