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