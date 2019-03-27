#include <unistd.h>
#include <sys/types.h>
#include <stdbool.h>
#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  char nargv[256][256];
  strcpy(nargv[0],"strace");
  for (int i=1;i<argc;++i) strcpy(nargv[i],argv[i]);
  nargv[argc] = NULL;
  for (int i=0;nargv[i]!=NULL;++i){
      printf("%s\n", nargv[i]);
  }
  if (execve("/usr/bin/strace", argv, NULL)==-1){
     perror("Execve Failed!");
     exit(1);
  }
  return 0;
}
