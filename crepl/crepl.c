#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <assert.h>

char buf[20000];
int main(int argc, char *argv[], char *env[]) {

    while (true){
      printf(">> ");
      scanf("%s", buf);

      char tmpname[]="tmpfileXXXXXX";
      int fd=mkstemp(tmpname);

      int pid = fork();
      int status;
      if (pid == 0){
        printf("Child:\n");
        char *argv_new[3];
        argv_new[0] = "/bin/cp";
        argv_new[1] = tmpname;
        argv_new[2] = "my.txt";
        execve("/bin/cp", argv_new, env);
      } else {
        int pid_ch = wait(&status);
        int ret = WEXITSTATUS(status);
        printf("child's pid=%d, exit = %d", pid_ch, ret);
      }

      unlink(tmpname);

      printf("  %s\n", buf);
    }
  return 0;
}
