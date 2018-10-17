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
