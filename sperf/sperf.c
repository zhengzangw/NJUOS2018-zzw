#include <unistd.h>
#include <sys/types.h>
#include <stdbool.h>
#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  char *nargv[256];
  nargv[0] = "/bin/sh";
  nargv[1] = "strace";
  nargv[2] = "-c";
  for (int i=3;i<argc+2;++i) nargv[i] = argv[i];
  if (execve("/bin/sh", argv, NULL)==-1){
     perror("Execve Failed!");
     exit(1);
  }
  return 0;
}
