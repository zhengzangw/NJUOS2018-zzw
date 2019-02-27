#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

typedef int bool;
#define true 1
#define false 0

char filename[512], procname[256];
bool visit[32768];

bool isnumber(char *s)
{
        int length = strlen(s);
        for (int i = 0; i < length; ++i) {
                if (!isdigit(s[i])) {
                        return false;
                }
        }
        return true;
}

struct Process {
        int pid, ppid, nson;
        char name[512];
        struct Process *son[128];
        bool isproc;
};

char tmp[1024];
void getinfo(struct Process *ret, int pid)
{
        char childfile[512], statname[512], taskdirname[512];
        sprintf(statname, "/proc/%d/stat", pid);
        sprintf(taskdirname, "/proc/%d/task/", pid);
        sprintf(childfile, "/proc/%d/task/%d/children", pid, pid);

        FILE *fp = fopen(statname, "r");
        fscanf(fp, "%d", &ret->pid);    // Get pid
        fscanf(fp, "%s", tmp);  // Get name
        tmp[strlen(tmp) - 1] = '\0';
        strcpy(ret->name, tmp + 1);
        fscanf(fp, "%s", tmp);
        fscanf(fp, "%d", &ret->ppid);   // Get ppid
        ret->isproc = true;
        fclose(fp);

        fp = fopen(childfile, "r");
        ret->nson = 0;
        int ch;
        while ((fscanf(fp, "%d", &ch)) != EOF) {
                ret->son[ret->nson] = malloc(sizeof(struct Process));
                getinfo(ret->son[ret->nson], ch);
                ret->nson++;
        }

        DIR *dir;
        struct dirent *ent;
        if ((dir = opendir(taskdirname)) != NULL) {
                while ((ent = readdir(dir)) != NULL) {
                        if (isnumber(ent->d_name)) {
                                int tid = atoi(ent->d_name);
                                if (tid != pid) {
                                        ret->son[ret->nson] =
                                            malloc(sizeof(struct Process));
                                        sprintf(tmp, "{%s}", ret->name);
                                        strcpy(ret->son[ret->nson]->name, tmp);
                                        ret->son[ret->nson]->pid = tid;
                                        ret->son[ret->nson]->isproc = false;
                                        ret->nson++;
                                }

                        }
                }
                closedir(dir);
        } else
                assert(0);
}

void search(struct Process *cur, int depth)
{
        printf("%*s%s(%d)\n", depth, "", cur->name, cur->pid);
        for (int i = 0; i < cur->nson; ++i) {
                if (cur->isproc)
                        search(cur->son[i], depth + 1);
        }
}

int main(int argc, char *argv[])
{
        // getopt
        bool issort = false, showpid = false;
        int opt;
        while ((opt = getopt(argc, argv, "Vnp")) != -1) {
                switch (opt) {
                case 'V':
                        printf
                            ("pstree 1.0\nCopyright (C) 2019 Zheng Zangwei\n");
                        return 0;
                case 'n':
                        issort = true;
                        break;
                case 'p':
                        showpid = true;
                default:
                        printf("Only -V,-n,-p is available.\n");
                        return -1;
                }
        }
        printf("%d%d\n", issort, showpid);

        struct Process *root = malloc(sizeof(struct Process));
        getinfo(root, 1);
        search(root, 0);
        return 0;
}
