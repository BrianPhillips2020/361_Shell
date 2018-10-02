#include "sh.h"
#include <signal.h>
#include <stdio.h>

void sig_handler(int signal);

pid_t childpid = 0;

int main( int argc, char **argv, char **envp )
{
  /* put signal set up stuff here */

  signal(SIGINT, sig_handler);
  //  signal(SIGALRM, sig_handler);
  //signal(SIGTERM, sig_handler);
  sigignore(SIGQUIT);
  sigignore(SIGTSTP);
  //  sigignore(SIGTERM);




  return sh(argc, argv, envp);
}

void sig_handler(int signal)
{
  switch(signal){
  case SIGINT:
    //printf("\n");
    if(childpid > 0){
      printf("\n");
      kill(childpid, SIGINT);
    }
    break;
  case SIGTERM:
    printf("\nsigkill\n");
    //    if(childpid > 0){
      printf("killing ...\n");
      kill(childpid, SIGKILL);
      //}
    printf("bitch wtf\n");
  }
}
