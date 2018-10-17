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