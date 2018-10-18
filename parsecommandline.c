
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
