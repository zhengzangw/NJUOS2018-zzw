#include <unistd.h>
#include <sys/types.h>
#include <stdbool.h>
#include <time.h>
#include <fcntl.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
  printf("%s\n", argv[1]);
  execvp("strace", argv+1);
  return 0;
}
