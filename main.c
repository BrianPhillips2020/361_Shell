#include "sh.h"
#include <signal.h>
#include <stdio.h>

void sig_handler(int signal);

pid_t childpid = 0;

int main( int argc, char **argv, char **envp )
{
  /* put signal set up stuff here */

  signal(SIGINT, sig_handler);
  sigignore(SIGQUIT);
  sigignore(SIGTSTP);





  sh(argc, argv, envp);

  return 0;
}

void sig_handler(int signal)
{
  switch(signal){
  case SIGINT:
    if(childpid > 0){
      printf("\n");
      kill(childpid, SIGINT);
    }
    break;
  case SIGTERM:
    if(childpid > 0){
      kill(childpid, SIGTERM);
    }

  }
}
