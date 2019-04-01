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
#include <time.h>

char buf[20000], buf2[20000], funcname[100], filename[100], cname[100], soname[100];
char *argv_new[20];
bool isfunc;
char wrapper[] = "__expr_wrap_";
int numfunc, numfile;

int main(int argc, char *argv[], char *env[])
{
        srand(time(NULL));
        argv_new[0] = "/usr/bin/gcc";
        argv_new[1] = sizeof(void *) == 4 ? "-m32" : "-m64";
        argv_new[2] = "-fPIC";
        argv_new[3] = "-shared";
        argv_new[4] = "-o";
        argv_new[7] = NULL;

        while (true) {
                printf(">> ");
                isfunc = 0;

                //Get input
                bzero(buf, sizeof(buf));
                fgets(buf, 10000, stdin);
                if (feof(stdin)) break;
                int b, s, t;
                for (b = 0; buf[b] == ' '; ++b) ;
                if (strcmp(buf + b, "exit\n") == 0) return 0;
                if (strncmp(buf + b, "int", 3) == 0) {
                        bzero(funcname, sizeof(funcname));
                        isfunc = 1;
                        for (s = b + 3; buf[s] == ' '; ++s) ;
                        for (t = s; buf[t] != '('; ++t) ;
                        strncpy(funcname, buf + s, t - s);
                } else {
                        buf[strlen(buf) - 1] = '\0';
                        bzero(buf2, sizeof(buf2));
                        sprintf(funcname, "%s%d", wrapper, numfunc++);
                        sprintf(buf2, "int %s(){return (%s);}", funcname, buf);
                        printf("%s\n", buf2);
                }

                //Prepare temp file
                sprintf(filename, "tmp%d", numfile++);
                sprintf(cname, "./%s.c", filename);
                sprintf(soname, "./%s.so", filename);
                FILE* fp = fopen(cname, "w");
                fprintf(fp, "%s\n", isfunc?buf:buf2);
                fclose(fp);
                argv_new[5] = soname;
                argv_new[6] = cname;

                int pid = fork();
                int status;
                if (pid == 0) {
                        close(STDERR_FILENO);
                        execve(argv_new[0], argv_new, env);
                } else {
                        //wait
                        wait(&status);
                        int ret = WEXITSTATUS(status);

                        // Deal with result
                        if (ret != 0) printf("  \033[31mCompile Error!\033[0m\n");
                        else {
                                int (*dfunc) (void);
                                void *dhandle = dlopen(soname, RTLD_NOW | RTLD_GLOBAL);
                                if (dhandle == NULL) {
                                        printf("  \033[31mCompile Error:\033[0m %s\n", dlerror());
                                } else {
                                        if (isfunc) {
                                                printf("  \033[32mAdded:\033[0m %s", buf);
                                        } else {
                                                printf("%s\n", funcname);
                                                dfunc = dlsym(dhandle, funcname);
                                                assert(dfunc != NULL);
                                                printf("  (%s) = %d.\n", buf, dfunc());
                                        }
                                        dlclose(dhandle);
                                }
                        }

                }

                unlink(cname);
                unlink(soname);
        }
        return 0;
}
