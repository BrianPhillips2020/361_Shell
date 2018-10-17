if(args[1] == NULL){
	  printf("Enter prompt prefix: ");
	  char tmp[PROMPTMAX];
	  fgets(tmp, PROMPTMAX, stdin);
	  tmp[(int) strlen(tmp) - 1] = '\0';
	  strcpy(prompt, tmp);
	}
	else{
	  strcpy(prompt, args[1]);
	}