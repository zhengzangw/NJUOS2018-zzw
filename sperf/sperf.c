#include <unistd.h>
#include <sys/types.h>
#include <stdbool.h>
#include <time.h>
#include <fcntl.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
  argv[0] = "strace";
  execve("/usr/bin/strace", argv, NULL);
  return 0;
}
