#include <unistd.h>
#include <sys/types.h>
#include <stdbool.h>
#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[], char *env[]) {
  //new argv
  /*
  char *argv_new[argc+2];
  argv_new[0] = "/usr/bin/strace";
  argv_new[1] = "-Txx";
  for (int i=1;i<argc;++i) argv_new[i+1] = argv[i];
  argv_new[argc+1] = NULL;
  */

  int flides[2];
  if (pipe(flides)!=0){
      perror("pipe failed");
      exit(EXIT_FAILURE);
  }
  int pid = fork();
  if (pid == 0){
     close(STDOUT_FILENO);
     dup2(flides[1], STDOUT_FILENO);
     //execve("/usr/bin/strace", argv_new, env);
     execlp("ls", "ls", -1, NULL);
  } else {
     close(STDIN_FILENO);
     dup2(flides[0], STDIN_FILENO);
     execlp("wc", "wc", "-l", NULL);
  }

  return 0;
}
