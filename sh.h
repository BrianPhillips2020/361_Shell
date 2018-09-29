#include "get_path.h"

int pid;
int sh(int argc, char **argv, char **envp);
char *which(char *command, struct pathelement *pathlist);
char *where(char *command, struct pathelement *pathlist);
void printenv(char **envp);
int strcmp_ignore_case(char const *a, char const *b);
int cd(char *command, char **args, char *homedir, char *currentdir);
int list(char *command, char **args);
int execute_command(char *command, char **args, char **envp, struct pathelement  *pathlist);


#define PROMPTMAX 32
#define MAXARGS 10
