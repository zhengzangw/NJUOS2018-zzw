#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

typedef int bool;
#define true 1
#define false 0

char filename[512], procname[256];
bool visit[32768];

bool isnumber(char *s, int length)
{
        for (int i = 0; i < length; ++i) {
                if (!(s[i] >= '0' && s[i] <= '9')) {
                        return false;
                }
        }
        return true;
}

void search(int cur, int depth)
{
        if (!visit[cur]) {
                visit[cur] = 1;

                sprintf(filename, "/proc/%d/comm", cur);
                FILE *fp = fopen(filename, "r");
                if (fp) {
                        fscanf(fp, "%s", procname);
                        fclose(fp);
                } else {
                        printf("Error on %s\n", filename);
                }

                printf("%*s%s(%d)\n", 4*depth, "", procname, cur);

                // get task
                DIR *dir;
                struct dirent *ent;
                sprintf(filename, "/proc/%d/task/", cur);
                if ((dir = opendir(filename)) != NULL) {
                        while ((ent = readdir(dir)) != NULL) {
                                if (isnumber(ent->d_name, strlen(ent->d_name))) {
                                        int curdd = atoi(ent->d_name);
                                        if (curdd != cur)
                                        printf("%*s%s(%d)\n", 4*(depth+1), "", procname, curdd);
                                }
                        }
                        closedir(dir);
                } else {
                        perror("");
                        return;
                }
                // get child process
                int ch;
                sprintf(filename, "/proc/%d/task/%d/children", cur, cur);
                fp = fopen(filename, "r");
                if (fp) {
                        while (fscanf(fp, "%d", &ch) != EOF) {
                                search(ch, depth + 1);
                        }
                        fclose(fp);
                } else {
                        printf("Error on %s\n", filename);
                }
        }
}

struct process {
    int pic,ppic;
    char name[512];
    struct process* son[128];
    bool isproc;
};
struct process* tree = new struct process;
struct process* cur  = tree;

void getinfo(int pid, process * cur, bool isproc){
    char statname[512], taskdirname[512], tmp[128];
    sprintf(statname, "/proc/%d/stat", pid);
    sprintf(taskdirname, "/proc/%d/task/", pid);
    FILE *fp = fopen(statname);
    fscanf(fp, "%d", &cur->pic);
    fscanf(fp, "%s", &tmp);
    tmp[strlen(tmp)-1]='\0';
    cur->name = tmp+1;
    fscanf(fp, "%s", tmp);
    fscanf(fp, "%d", &cur->ppic)
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

        getinfo(fp, cur, true);
        printf("pid=%d\nppid=%d\n%s\n", pid, ppid, name);
/*
        DIR *dir;
        struct dirent *ent;
        if ((dir = opendir("/proc/")) != NULL) {
                while ((ent = readdir(dir)) != NULL) {
                        if (isnumber(ent->d_name, strlen(ent->d_name))) {
                                int cur = atoi(ent->d_name);
                                search(cur, 0);
                        }
                }
                closedir(dir);
        } else {
                perror("");
                return -1;
        }
        */
        return 0;
}
