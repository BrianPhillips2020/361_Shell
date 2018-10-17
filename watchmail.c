
	if(args[2] == NULL){
	  //two arguemnts, meaning start watching one file
	  struct stat buff;
	  int exists = stat(args[1], &buff);
	  printf("exists? %d\n", exists);
	  if(exists == 0){
	    pthread_t mail_t;

	    char *filepath = malloc(sizeof(char) * strlen(args[1]));
	    strcpy(filepath, args[1]);
	    printf("%s\n", filepath);
	    pthread_create(&mail_t, NULL, watchmail, (void *)filepath);
	    
	    if(mailthread == 0 || watchmailhead == NULL){
	      mailthread = 1;
	      watchmailhead = malloc(sizeof(struct maillist));
	      watchmailhead->str = malloc(sizeof(char) * strlen(filepath));
	      strcpy(watchmailhead->str, filepath);
	      watchmailhead->id = mail_t;
	      watchmailhead->next = NULL;
	    }else{
	      struct maillist *tmp = watchmailhead;
	      while(tmp->next != NULL){
		tmp = tmp->next;
	      }
	      tmp->next = malloc(sizeof(struct maillist));
	      tmp->next->str = malloc(sizeof(char) * strlen(filepath));
	      strcpy(tmp->next->str, filepath);
	      tmp->next->id = mail_t;
	      tmp->next->next = NULL;
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
	    tmp->next = NULL;
	  }else{
	    //Remove another node from watchlist
	    struct maillist *tmp2 = watchmailhead;
	    while(strcmp(tmp2->next->str, args[1]) != 0){
	      tmp2 = tmp2->next;
	    }
	    if(strcmp(tmp2->next->str, args[1]) == 0){
	      pthread_cancel(tmp2->next->id);
	      printf("joining thread\n");
	      int j = pthread_join(tmp2->next->id, NULL);
	      printf("joined? %d\n", j);
	      tmp2->next = tmp2->next->next;
	    }else{
	      printf("File not being watched\n");
	    }
	  }
	}
