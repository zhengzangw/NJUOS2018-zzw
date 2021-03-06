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
bool issort = false, showpid = false;

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
        int pid, ppid, nson, nthr;
        char name[512];
        struct Process *son[128];
        struct Process *thr[128];
};

char tmp[2048];
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
        ret->nthr = 0;
        if ((dir = opendir(taskdirname)) != NULL) {
                while ((ent = readdir(dir)) != NULL) {
                        if (isnumber(ent->d_name)) {
                                int tid = atoi(ent->d_name);
                                if (tid != pid) {
                                    if (showpid){
                                        ret->thr[ret->nthr] =
                                            malloc(sizeof(struct Process));
                                        sprintf(tmp, "{%s}", ret->name);
                                        strcpy(ret->thr[ret->nthr]->name, tmp);
                                        ret->thr[ret->nthr]->pid = tid;
                                        ret->thr[ret->nthr]->nthr =
                                            ret->thr[ret->nthr]->nson = 0;
                                    }
                                        ret->nthr++;
                                }
                                if (!showpid && ret->nthr >0){
                                        ret->thr[0] =
                                            malloc(sizeof(struct Process));
                                        sprintf(tmp, "{%s}", ret->name);
                                        strcpy(ret->thr[0]->name, tmp);
                                        ret->thr[0]->pid = ret->nthr;
                                        ret->nthr = 1;
                                        ret->thr[0]->nthr =
                                            ret->thr[0]->nson = 0;
                                }

                        }
                }
                closedir(dir);
        } else
                assert(0);
}

bool cpr(char *a, char *b){
    int lena = strlen(a), lenb = strlen(b);
    int len = lena < lenb ?lena : lenb;
    for (int i=0;i<len;++i){
        if (a[i]>b[i]) return true;
        if (b[i]>a[i]) return false;
    }
    return lena>lenb;
}

char pre[512] = "";
int stack[512];
int head = 0;
bool isroot = true;
void search(struct Process *cur, int type, bool isproc)
{
        if (isroot) {
            if (showpid)
                sprintf(tmp, "%s(%d)", cur->name, cur->pid);
            else{
                sprintf(tmp, "%s", cur->name);
            }
                isroot = false;
                stack[++head] = strlen(tmp) + 1;
        } else {
                switch (type) {
                case 0:        //first of one subtree
            if (showpid)
                sprintf(tmp, "-%s(%d)", cur->name, cur->pid);
            else{
                sprintf(tmp, "-%s", cur->name);
                }
                        stack[head + 1] = stack[head] + strlen(tmp) + 1;
                        head++;
                        break;
                case -1:
                        pre[stack[head] - 1] = '\\';
                default:
                if (showpid)
                        sprintf(tmp, "%s-%s(%d)", pre, cur->name, cur->pid);
                else{
                    sprintf(tmp, "%s-%s", pre, cur->name);
                    }
                        stack[++head] = strlen(tmp) + 1;
                }
        }

        printf("%s", tmp);
        if (type == -1)
                pre[stack[head - 1] - 1] = ' ';

        for (int i = stack[head - 1]; i < stack[head]; ++i)
                pre[i] = ' ';
        pre[stack[head]] = '|';

        switch (cur->nson + cur->nthr) {
        case 0:
                printf("\n");
                break;
        case 1:
                printf("--");
                pre[stack[head]] = ' ';
                break;
        default:
                printf("-+");
        }

        pre[++stack[head]] = '\0';

        if (isproc) {
            if (issort){
                for (int i=0; i<cur->nson; ++i)
                    for (int j=i+1;j<cur->nson; ++j)
                        if (cur->son[i]->pid>cur->son[j]->pid){
                            struct Process * temp = cur->son[i];
                            cur->son[i] = cur->son[j];
                            cur->son[j] = temp;
                        }
            } else {
                for (int i=0; i<cur->nson; ++i)
                    for (int j=i+1;j<cur->nson; ++j)
                        if (cpr(cur->son[i]->name,cur->son[j]->name)){
                            struct Process * temp = cur->son[i];
                            cur->son[i] = cur->son[j];
                            cur->son[j] = temp;
                        }
            }
                for (int i = 0; i < cur->nson; ++i) {
                        int ith = i;
                        if (cur->nson > 1 && cur->nthr == 0
                            && cur->nson - 1 == i)
                                ith = -1;
                        search(cur->son[i], ith, true);
                }
            if (showpid){
                for (int i = 0; i < cur->nthr; ++i) {
                        int ith = i + cur->nson;
                        if (cur->nson + cur->nthr > 1
                            && cur->nthr - 1 + cur->nson == ith)
                                ith = -1;
                        search(cur->thr[i], ith, false);
                }
            } else if (cur->nthr>0){
                if (cur->thr[0]->pid>1){
                sprintf(tmp, "%d*[%s]", cur->thr[0]->pid, cur->thr[0]->name);
                strcpy(cur->thr[0]->name, tmp);
                }
                int ith;
                if (cur->nson==0) ith = 0; else ith = -1;
                search(cur->thr[0], ith, false);
            }
        }
        head--;
        pre[stack[head]] = '\0';

}

int main(int argc, char *argv[])
{
        // getopt
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
                        break;
                default:
                        printf("Only -V,-n,-p is available.\n");
                        return -1;
                }
        }

        struct Process *root = malloc(sizeof(struct Process));
        getinfo(root, 1);
        search(root, 0, true);
        return 0;
}
