#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

char buf[20000];
char *argv_new[10];
int main(int argc, char *argv[], char *env[]) {

    while (true){
      printf(">> ");
      fgets(buf, 10000, stdin);

      //Prepare temp file
      char tmpname[]="tmpfileXXXXXX";
      int fd=mkstemp(tmpname);
      write(fd, buf, 10000);
      char tmpo[30];
      strcpy(tmpo, tmpname);
      strcat(tmpo, ".o");

      //Prepare Varible
      argv_new[0] = "/usr/bin/gcc";
      argv_new[1] = tmpname;
      argv_new[2] = "-o";
      argv_new[3] = tmpo;
      argv_new[4] = NULL;

      int pid = fork();
      int status;
      if (pid == 0){
        execve("/bin/cp", argv_new, env);
      } else {
        int pid_ch = wait(&status);
        int ret = WEXITSTATUS(status);

        printf("(main)child's pid=%d, exit = %d\n", pid_ch, ret);
      }

      unlink(tmpname);

      printf("  %s\n", buf);
    }
  return 0;
}
