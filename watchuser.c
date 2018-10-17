
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