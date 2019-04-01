#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <dlfcn.h>
#include <fcntl.h>

char buf[20000],buf2[20000],funcname[100];
char *argv_new[20];
bool isfunc;
char wrapper[] = "__expr_wrap_123";
int main(int argc, char *argv[], char *env[]) {

    while (true){
      printf(">> ");
      isfunc = 0;

      //Get input
      bzero(buf, sizeof(buf));
      fgets(buf, 10000, stdin);
      if (feof(stdin)) break;
      int b;
      for (b=0;buf[b]==' ';++b);
      if (strncmp(buf+b,"int", 3)==0) {
        puts("isfunc");
        bzero(funcname, sizeof(funcname));
        isfunc = 1;
        int s,t;
        for (s=b+3;buf[s]==' ';++s);
        for (t=s;buf[t]!='(';++t);
        strncpy(funcname, buf+s, t-s);
      } else {
        buf[strlen(buf)-1] = '\0';
        bzero(buf2, sizeof(buf2));
        sprintf(buf2, "int %s(){return (%s);}", wrapper, buf);
        printf("%s\n", buf2);
      }

      //Prepare temp file
      char tmpname[]="tmpfileXXXXXX";
      int fd=mkstemp(tmpname);
      write(fd, isfunc?buf:buf2, 10000);

      //Prepare Varible
      char tmpo[]="./tmpfile.so";
      argv_new[0] = "/usr/bin/gcc";
      argv_new[1] = sizeof(void *)==4?"-m32":"-m64";
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
        //close(STDERR_FILENO);
        execve(argv_new[0], argv_new, env);
      } else {
        //wait
        wait(&status);
        int ret = WEXITSTATUS(status);

        // Deal with result
        if (ret!=0) printf("  Compile Error!\n");
        else {
            int (*dfunc)(void);
            void *dhandle = dlopen(tmpo, RTLD_NOW|RTLD_GLOBAL);
            if (dhandle==NULL){
              printf("%s\n", dlerror());
              exit(1);
            }
            if (isfunc){
              dfunc = dlsym(dhandle, funcname);
              assert(dfunc!=NULL);
              printf("  Added: %s", buf);
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
