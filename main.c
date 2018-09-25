#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define BUFFERSIZE 124

int main(int argc, char *argv[]){
  char str[BUFFERSIZE];
  char exit[4];
  strcpy(exit, "exit");
  while(strncmp(str, exit, 4) != 0){
    printf(">");
    fgets(str, BUFFERSIZE, stdin);
   }
  printf("Exiting Shell...\n");
  return 0;
}
