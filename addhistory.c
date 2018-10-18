        if(current_length == 0){
          histhead = malloc(sizeof(struct history));
          histhead->commandline = malloc(sizeof(char) * strlen(buffer) + 1);
          strcpy(histhead->commandline, buffer);
          histtail = histhead;
          histhead->next = NULL;
          histhead->prev = NULL;
          current_length++;
        }
        else{
          struct history *tmp;
          tmp = malloc(sizeof(struct history));
          tmp->commandline = malloc(sizeof(char) * strlen(buffer) + 1);
          strcpy(tmp->commandline, buffer);
          tmp->next = histhead;
          tmp->prev = NULL;
          histhead->prev = tmp;
          histhead = tmp;
          current_length++;
        }