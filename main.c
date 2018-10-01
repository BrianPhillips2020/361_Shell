#include "sh.h"
#include <signal.h>
#include <stdio.h>

void sig_handler(int signal);

pid_t c_pid = 0;

int main( int argc, char **argv, char **envp )
{
  /* put signal set up stuff here */

  signal(SIGINT, sig_handler);
  signal(SIGALRM, sig_handler);
  sigignore(SIGQUIT);
  sigignore(SIGTSTP);
  sigignore(SIGTERM);




  return sh(argc, argv, envp);
}

void sig_handler(int signal)
{
  switch(signal){
  case SIGINT:
    if(c_pid > 0){
      printf("\n");
      kill(c_pid, SIGINT);
    }
    break;
  case SIGALRM:
    if(c_pid > 0){
      kill(c_pid, SIGKILL);
    }
    break;
  case SIGKILL:
    if(c_pid > 0){
      kill(c_pid, SIGKILL);
    }
  }
}
