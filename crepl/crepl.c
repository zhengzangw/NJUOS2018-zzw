#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

char buf[20000];
int main(int argc, char *argv[]) {

    while (true){
      printf(">> ");
      scanf("%s", buf);

      FILE *tmpfp = tmpfile();
      if (tmpfp==NULL){
          perror("tmpfile error");
          return 1;
      }
      fputs(buf, tmpfp);

      int pid = fork();
      int status;
      if (pid == 0){
        printf("Hello");
        exit(0);
      } else {
        int pid_ch = wait(&status);
        int ret = WEXITSTATUS(status);
        printf("child's pid=%d, exit = %d", pid_ch, ret);
      }


      printf("  %s\n", buf);
    }
  return 0;
}
