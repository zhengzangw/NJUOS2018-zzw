#include <devices.h>
#include <common.h>
#include <vfs.h>

#define modify(file)\
    if (file[0]!='/') {\
        char tmpname[128];\
        strcpy(tmpname, file);\
        sprintf(file, "%s%s", pwd, tmpname);\
    }

#define get1arg(len)\
    int nofile = 1;\
    if (line[len]==' '){\
        nofile = 0;\
        strcpy(arg1, line+len+1);\
    }

#define get2arg(len)\
    int nofile = 1;\
    if (line[len]==' '){\
        int slen;\
        for (slen = len+1;line[slen]!='\0';slen++){\
            if (line[slen]==' ') break;\
        }\
        if (line[slen]==' ') nofile = 0;\
        strncpy(arg1, line+len+1, slen-len-1);\
        strcpy(arg2, line+slen+1);\
    }

#define SUCCESS "[SUCCESS]: "
#define FAIL "[FAIL]: "
#define iscmd(cmd, length) (strncmp(line, cmd, len=length)==0)
#define ifnoarg_do if (nofile) strcpy(text, FAIL "missing operand\n");
#define ifnofd_do(file) if (fd<0) sprintf(text, FAIL "no such file or directory %s\n", file);
#define ifnotDR_do(file) if (FILE(fd)->inode->type!=DR) sprintf(text, FAIL "not a directory: %s\n", file);
#define ifnotNF_do(file) if (FILE(fd)->inode->type==DR) sprintf(text, FAIL "Is a directory: %s\n", file);

void shell_task(void *name){
    char pwd[128];
    strcpy(pwd, "/");
    char *shell_name = filename(name);
    char line[128], text[1024], arg1[128], arg2[128];
    int len;

    int task_fd = vfs->open(name, 0);
    while (1){
        memset(arg1, 0, sizeof(arg1));
        memset(arg2, 0, sizeof(arg2));
        sprintf(text, "(%s):%s $ ", shell_name, pwd);

        vfs->write(task_fd, text, strlen(text));
        int nread = vfs->read(task_fd, line, sizeof(line));
        line[nread-1] = '\0';

        if (strcmp(line, "pwd")==0){
            sprintf(text, "%s\n", pwd);
        } else if (strcmp(line, "exit")==0){
            _halt(0);
        } else if (iscmd("cd", 2)){
            int ret = 0;
            if (line[len]!=' ') {
                strcpy(pwd, "/");
            }
            else {
                strcpy(arg1, line+len+1);
                int ptr = 0;
                if (arg1[0] == '/'){
                    ptr = 1;
                    strcpy(pwd, "/");
                }
                while (ptr<strlen(arg1)){
                    if (arg1[ptr]=='.'){
                        if (arg1[ptr+1]=='.'){
                            if (strlen(pwd)>1){
                                for (int i=strlen(pwd)-2;i>=0;--i){
                                    if (pwd[i]=='/'){
                                        pwd[i+1]='\0';
                                        break;
                                        }
                                }
                            }
                            ptr+=3;
                        } else {
                            ptr+=2;
                        }
                    } else {
                            int oldptr = ptr;
                            while (arg1[ptr]!='/' && ptr<strlen(arg1)) ptr++;
                            char tmpfile[128], tmppwd[128];
                            strncpy(tmpfile, arg1+oldptr, ptr-oldptr);
                            tmpfile[ptr-oldptr] = '\0';
                            sprintf(tmppwd, "%s%s", pwd, tmpfile);
                            int fd = vfs->open(tmppwd, O_RD);
                            if (fd>=0) {
                                if (FILE(fd)->inode->type!=DR) ret = -2;
                                else {
                                    vfs->close(fd);
                                    sprintf(pwd, "%s/", tmppwd);
                                }
                            } else {
                                ret = -1;
                                break;
                            }
                            ptr++;
                    }
                }
            }
            switch (ret){
                case 0:
                    sprintf(text, SUCCESS "change director to %s\n", pwd);
                    break;
                case -1:
                    sprintf(text, FAIL "no such file or directory: %s\n", arg1);
                    break;
                case -2:
                    sprintf(text, FAIL "not a directory: %s\n", arg1);
            }
        } else if (iscmd("stat", 4)){
            get1arg(4);
            ifnoarg_do
            else {
                modify(arg1);
                int fd = vfs->open(arg1, O_RD);
                ifnofd_do(arg1)
                else {
                    char typename[10];
                    char additional[20];
                    switch (FILE(fd)->inode->type){
                        case DR:
                            strcpy(typename, "directory");
                            sprintf(additional, "Number of Files: %d\n", FILE(fd)->inode->dir_len-2);
                        break;
                        case NF:
                            strcpy(typename, "normal file");
                            sprintf(additional, "\0");
                            break;
                        case DV_CHAR:
                            strcpy(typename, "char device");
                            sprintf(additional, "\0");
                            break;
                        case DV_BLOCK:
                            strcpy(typename, "block device");
                            sprintf(additional, "\0");
                            break;
                        }
                        sprintf(text, "  File: %s\n  Type: %s\n  Size: %d\nDevice: %s\nAccess: %x\n    ID: %d\n Links: %d\n%s", arg1,
                            typename,
                            FILE(fd)->inode->size,
                            FILE(fd)->inode->fs->dev->name,
                            FILE(fd)->inode->permission,
                            FILE(fd)->inode->id,
                            FILE(fd)->inode->link_num,
                            additional
                        );
                        vfs->close(fd);
                    }
            }
        } else if (iscmd("ls", 2)){
            get1arg(2);
            if (nofile) strcpy(arg1, pwd);
            modify(arg1);
            int fd = vfs->open(arg1, O_RD);

            ifnofd_do(arg1)
            else ifnotDR_do(arg1)
            else {
                    memset(text, 0, 128);;
                    vfs->read(fd, text, 128);
                    vfs->close(fd);
                }
        } else if (iscmd("mkdir", 5)){
            get1arg(5);
            ifnoarg_do
            else {
                modify(arg1);
                vfs->mkdir(arg1);
                sprintf(text, SUCCESS "create %s\n", arg1);
            }
        } else if (iscmd("rmdir", 5)){
            get1arg(5);
            ifnoarg_do
            else {
                modify(arg1);
                int fd = vfs->open(arg1, 0);
                ifnofd_do(arg1)
                else {
                    ifnotDR_do(arg1)
                    else {
                        vfs->close(fd);
                        int ret = vfs->rmdir(arg1);
                        if (ret==0) {
                            sprintf(text, SUCCESS "remove %s\n", arg1);
                        } else {
                            sprintf(text, FAIL "fail to remove %s\n", arg1);
                        }
                    }
                }
            }
        } else if (iscmd("rm", 2)){
            get1arg(2);
            ifnoarg_do
            else {
                modify(arg1);
                int fd = vfs->open(arg1, 0);
                ifnofd_do(arg1)
                else ifnotNF_do(arg1)
                else {
                        vfs->close(fd);
                        int ret = vfs->unlink(arg1);
                        if (ret==0) {
                            sprintf(text, SUCCESS "remove %s\n", arg1);
                        } else {
                            sprintf(text, FAIL "fail to remove %s\n", arg1);
                        }
                     }
            }
        } else if (iscmd("touch", 5)){
            get1arg(5);
            ifnoarg_do
            else {
                modify(arg1);
                int fd = vfs->open(arg1, 0);
                if (fd>=0) {
                    sprintf(text, FAIL "file already exists %s\n", arg1);
                } else {
                    int fd = vfs->open(arg1, O_CREAT);
                    if (fd>=0){
                        vfs->close(fd);
                        sprintf(text, SUCCESS "create file %s\n", arg1);
                    } else {
                        sprintf(text, FAIL "cannot create file %s\n", arg1);
                    }
                }
            }
        } else if (iscmd("cat", 3)) {
            get1arg(3);
            ifnoarg_do
            else {
                modify(arg1);
                int fd = vfs->open(arg1, O_RD);
                ifnofd_do(arg1)
                else ifnotNF_do(arg1)
                else {
                    memset(text, 0, 1024);;
                    int bytes = vfs->read(fd, text, 1024);
                    if (bytes<0){
                        sprintf(text, FAIL "cannot read to file with fd=%d\n", fd);
                    }
                    vfs->close(fd);
                }
            }
        } else if (iscmd("link", 4)){
            get2arg(4)
            ifnoarg_do
            else {
                modify(arg1);
                modify(arg2);
                int ret = vfs->link(arg1, arg2);
                if (ret == -1){
                    sprintf(text, FAIL "cannot create link to file %s\n", arg1);
                } else {
                    sprintf(text, SUCCESS "create link %s -> %s\n", arg2, arg1);
                }
            }
        } else if (iscmd("open", 4)){
            get1arg(4);
            ifnoarg_do
            else {
                modify(arg1);
                int fd = vfs->open(arg1, 0);
                ifnofd_do(arg1)
                else ifnotNF_do(arg1)
                else sprintf(text, SUCCESS "open file %s -> fd=%d\n", arg1, fd);
            }
        } else if (iscmd("close", 5)){
            get1arg(5);
            ifnoarg_do
            else{
                int fd = atoi(arg1);
                if (vfs->close(fd)==0)
                    sprintf(text, SUCCESS "close file with fd=%d\n", fd);
                else sprintf(text, FAIL "unable to close file with fd=%d\n");
            }
        } else if (iscmd("write", 5)){
            get2arg(5);
            ifnoarg_do
            else{
                int fd = atoi(arg1);
                int bytes = vfs->write(fd, arg2, strlen(arg2));
                if (bytes>=0){
                    sprintf(text, SUCCESS "write %d bytes to file with fd=%d\n", bytes, fd);
                } else {
                    sprintf(text, FAIL "cannot write to file with fd=%d\n", fd);
                }
            }
        } else if (iscmd("read", 4)){
            get1arg(4);
            ifnoarg_do{
                int fd = atoi(arg1);
                int bytes = vfs->read(fd, text, 1024);
                if (bytes<0){
                    sprintf(text, FAIL "cannot read to file with fd=%d\n", fd);
                }
            }
        } else if (iscmd("lkset", 5)){
            get2arg(5);
            ifnoarg_do
            else {
                int fd = atoi(arg1);
                int offset = atoi(arg2);
                off_t of = vfs->lseek(fd, offset, S_SET);
                if (of<0) sprintf(text, "cannot set offset for file with fd=%d\n", fd);
                else sprintf(text, "set offset to %d for file with fd=%d\n", of, fd);
            }
        } else if (iscmd("lkcur", 5)){
            get2arg(5);
            ifnoarg_do
            else {
                int fd = atoi(arg1);
                int offset = atoi(arg2);
                off_t of = vfs->lseek(fd, offset, S_CUR);
                if (of<0) sprintf(text, "cannot set offset for file with fd=%d\n", fd);
                else sprintf(text, "set offset to %d for file with fd=%d\n", of, fd);
            }
        } else if (iscmd("lkend", 5)){
            get2arg(5);
            ifnoarg_do
            else {
                int fd = atoi(arg1);
                int offset = atoi(arg2);
                off_t of = vfs->lseek(fd, offset, S_END);
                if (of<0) sprintf(text, "cannot set offset for file with fd=%d\n", fd);
                else sprintf(text, "set offset to %d for file with fd=%d\n", of, fd);
            }
        } else if (strcmp("mount", line)==0){
            memset(text, sizeof(text), 0);
            sprintf(text, "PATH        DEVICE\n");
            for (int i=0;i<MAXMP;++i){
                if (mpt[i].exists){
                    sprintf(text+strlen(text), "%-12s%s\n", mpt[i].path, mpt[i].fs->dev->name);
                }
            }
        } else {
            sprintf(text, FAIL "command not found %s\n", line);
        }

        vfs->write(task_fd, text, strlen(text));
    }
}
