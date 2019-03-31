#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <dlfcn.h>

char buf[20000],buf2[20000];
char *argv_new[20];
bool isfunc;
char wrapper[] = "__expr_wrap_123";
int main(int argc, char *argv[], char *env[]) {

    while (true){
      printf(">> ");

      //Get input
      fgets(buf, 10000, stdin);
      if (feof(stdin)) break;
      if (strncmp(buf,"int", 3)==0) isfunc = 1;
      else {
        sprintf(buf2, "int %s(){return (%s);}", wrapper, buf);
        buf[strlen(buf)-1] = '\0';
      }

      //Prepare temp file
      char tmpname[]="tmpfileXXXXXX";
      int fd=mkstemp(tmpname);
      write(fd, isfunc?buf:buf2, 10000);

      //Prepare Varible
      char tmpo[]="./tmpfile.so";
      argv_new[0] = "/usr/bin/gcc";
      argv_new[1] = "-x";
      argv_new[2] = "c";
      argv_new[3] = "-fPIC";
      argv_new[4] = "-shared";
      argv_new[5] = tmpname;
      argv_new[6] = "-o";
      argv_new[7] = tmpo;
      argv_new[8] = "-ldl";
      argv_new[9] = "-w";
      argv_new[10] = NULL;

      int pid = fork();
      int status;
      if (pid == 0){
        close(STDERR_FILENO);
        execve(argv_new[0], argv_new, env);
      } else {
        //wait
        int pid_ch = wait(&status);
        int ret = WEXITSTATUS(status);

        // Deal with result
        if (ret!=0) printf("  Compile Error!\n");
        else {
            int (*dfunc)(void);
            void *dhandle = dlopen(tmpo, RTLD_LAZY|RTLD_GLOBAL);
            assert(dhandle!=NULL);
            if (isfunc){
              dfunc = dlsym(dhandle, "a");
              assert(dfunc!=NULL);
              printf("Added: %s\n", buf);
            } else {
              dfunc = dlsym(dhandle, wrapper);
              assert(dfunc!=NULL);
              printf("  (%s) = %d.\n", buf, dfunc());
            }
            dlclose(dhandle);
        }

        //printf("(main)child's pid=%d, exit = %d\n", pid_ch, ret);
      }

      unlink(tmpname);
      unlink(tmpo);
    }
  return 0;
}
