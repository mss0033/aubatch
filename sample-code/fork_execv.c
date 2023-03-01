#include <stdio.h> 
#include <stdlib.h> 
#include <sys/types.h>
#include <unistd.h> 

int main(void)
{
  char *my_args[5];
  pid_t pid;
  
  my_args[0] = "process";
  my_args[1] = "-help";
  my_args[2] = "-setup";
  my_args[3] = NULL;
  
  puts("fork()ing");
  
  switch ((pid = fork()))
  {
    case -1:
      /* Fork() has failed */
      perror("fork");
      break;
    case 0:
      /* This is processed by the child */
      execv("process", my_args);
      puts("Uh oh! If this prints, execv() must have failed");
      exit(EXIT_FAILURE);
      break;
    default:
      /* This is processed by the parent */
      puts ("This is a message from the parent");
      break;
  }
  
  puts ("End of parent program");
  return 0;
}

