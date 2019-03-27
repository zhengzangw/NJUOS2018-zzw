#include <unistd.h>
#include <sys/types.h>
#include <stdbool.h>
#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  argv[0] = "strace";
  if (execve("/usr/bin/strace", argv, NULL)==-1){
     perror("Execve Failed!");
     exit(1);
  }
  return 0;
}
