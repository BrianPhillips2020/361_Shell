#include "get_path.h"
#include <pthread.h>

int pid;
int sh(int argc, char **argv, char **envp);
char *which(char *command, struct pathelement *pathlist);
char *where(char *command, struct pathelement *pathlist);
int strcmp_ignore_case(char const *a, char const *b);
int cd(char *command, char **args, char *homedir, char *currentdir, char *previousdir);
int list(char *command, char **args, char *currentdir);
int execute_command(char *command, char **args, char **envp, struct pathelement  *pathlist);
int killsig(char *command, char **args);
void printenv(char **args, char **envp);
void *watchuser(void *arg);
void *watchmail(void *arg);

struct history{

  char *commandline;
  struct history *next;
  struct history *prev;
};

struct maillist{
  char *str;
  pthread_t id;
  struct maillist *next;
};
  
struct strlist{
  char *str;
  int status;
  struct strlist *next;
  struct strlist *prev;
};

#define ABUFFER 20

struct alias_entry{
  char *key;
  char *command;
  struct alias_entry *next;
  struct alias_entry *prev;
};



#define PROMPTMAX 64
#define MAXARGS 10
