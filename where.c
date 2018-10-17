if(args[1] != NULL){
	  for(int j = 1; args[j] != NULL; j++){
	    where(args[j], pathlist);
	  }
	}
	else{
	  printf("Usage for where: where [command1] ...\n");
	}