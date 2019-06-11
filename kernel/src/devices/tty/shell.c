#include <devices.h>
#include <common.h>
#include <vfs.h>

#define modify(file)\
    if (file[0]!='/') {\
        char tmpname[128];\
        strcpy(tmpname, file);\
        sprintf(file, "%s%s", pwd, tmpname);\
    }

#define getname(len)\
    int nofile = 1;\
    if (line[len]==' '){\
        nofile = 0;\
        strcpy(file, line+len+1);\
        modify(file)\
    }
#define gettwoname(len)\
    int nofile = 1;\
    if (line[len]==' '){\
        int slen;\
        for (slen = len+1;line[slen]!='\0';slen++){\
            if (line[slen]==' ') break;\
        }\
        if (line[slen]==' ') nofile = 0;\
        strncpy(file, line+len+1, slen-len-1);\
        strcpy(file2, line+slen+1);\
        modify(file);\
        modify(file2);\
    }

#define SUCCESS "[SUCCESS]: "
#define FAIL "[FAIL]: "

static int isdigit(int ch){return (ch>='0')&&(ch<='9');}
static int my_atoi(char *s){
    int i=0, flag = 1;
    char *ptr = s;
    if (*ptr == '-') {
        ptr++;
        flag = -1;
    }
    while (isdigit(*ptr)){
        i = i*10+*ptr-'0';
        ptr++;
    }
    return i*flag;
}

void shell_task(void *name){
    char pwd[128];
    strcpy(pwd, "/");
    device_t *tty = dev_lookup(name);
    while (1){
        char line[128], text[1024], file[128], file2[128];
        memset(file, 0, sizeof(file));
        memset(file2, 0, sizeof(file2));
        sprintf(text, "(%s):%s $ ", name, pwd);
        tty_write(tty, 0, text, strlen(text));
        int nread = tty->ops->read(tty, 0, line, sizeof(line));
        line[nread-1] = '\0';

        if (strcmp(line, "pwd")==0){
            sprintf(text, "%s\n", pwd);
        } else if (strcmp(line, "exit")==0){
            _halt(0);
        } else if (strncmp(line, "cd", 2)==0){
            int len = 2, ret = 0;
            if (line[2]!=' ') {
                strcpy(pwd, "/");
            }
            else {
                strcpy(file, line+len+1);
                int ptr = 0;
                if (file[0] == '/'){
                    ptr = 1;
                    strcpy(pwd, "/");
                }
                while (ptr<strlen(file)){
                    if (file[ptr]=='.'){
                        if (file[ptr+1]=='.'){
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
                            while (file[ptr]!='/' && ptr<strlen(file)) ptr++;
                            char tmpfile[128], tmppwd[128];
                            strncpy(tmpfile, file+oldptr, ptr-oldptr);
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
                    sprintf(text, FAIL "no such file or directory: %s\n", file);
                    break;
                case -2:
                    sprintf(text, FAIL "not a directory: %s\n", file);
            }
        } else if (strncmp(line, "stat", 4)==0){
            getname(4);
            if (nofile) strcpy(text, FAIL "missing operand\n");
            else {
                int fd = vfs->open(file, O_RD);
                    if (fd<0){
                        sprintf(text, FAIL "no such file or directory %s\n", file);
                    } else {

                        char typename[10];
                        char additional[20];
                        switch (cputask[_cpu()]->flides[fd]->inode->type){
                            case DR:
                                strcpy(typename, "directory");
                                sprintf(additional, "Number of Files: %d\n", FILE(fd)->inode->dir_len-2);
                                break;
                            default:
                                strcpy(typename, "normal file");
                                sprintf(additional, "\0");
                        }
                        sprintf(text, "  File: %s\n  Type: %s\n  Size: %d\nDevice: %s\nAccess: %x\n    ID: %d\n Links: %d\n%s", file,
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
        } else if (strncmp(line, "ls", 2)==0){
            getname(2);
            if (nofile) strcpy(file, pwd);
            int fd = vfs->open(file, O_RD);
            if (fd<0){
                sprintf(text, FAIL "no such file or directory %s\n", file);
            } else {
                if (FILE(fd)->inode->type!=DR){
                    sprintf(text, FAIL "not a directory: %s\n", file);
                } else {
                    memset(text, 0, 128);;
                    vfs->read(fd, text, 128);
                    vfs->close(fd);
                }
            }
        } else if (strncmp(line, "mkdir", 5)==0){
            getname(5);
            if (nofile) strcpy(text, FAIL "missing operand\n");
            else {
                vfs->mkdir(file);
                sprintf(text, SUCCESS "create %s\n", file);
            }
        } else if (strncmp(line, "rmdir", 5)==0){
            getname(5);
            if (nofile) strcpy(text, FAIL "missing operand\n");
            else {
                int fd = vfs->open(file, 0);
                if (fd<0) {
                    sprintf(text, FAIL "no such file or directory\n");
                } else {
                    if (FILE(fd)->inode->type!=DR){
                        sprintf(text, FAIL "not a directory: %s\n", file);
                    } else {
                        vfs->close(fd);
                        int ret = vfs->rmdir(file);
                        if (ret==0) {
                            sprintf(text, SUCCESS "remove %s\n", file);
                        } else {
                            sprintf(text, FAIL "fail to remove %s\n", file);
                        }
                    }
                }
            }
        } else if (strncmp(line, "rm", 2)==0){
            getname(2);
            if (nofile) strcpy(text, FAIL "missing operand\n");
            else {
                int fd = vfs->open(file, 0);
                if (fd<0) {
                    sprintf(text, FAIL "no such file or directory\n");
                } else {
                    if (FILE(fd)->inode->type!=NF){
                        sprintf(text, FAIL "Is a directory: %s\n", file);
                    } else {
                        vfs->close(fd);
                        int ret = vfs->unlink(file);
                        if (ret==0) {
                            sprintf(text, SUCCESS "remove %s\n", file);
                        } else {
                            sprintf(text, FAIL "fail to remove %s\n", file);
                        }
                    }
                }
            }
        } else if (strncmp(line, "touch", 5)==0){
            getname(5);
            if (nofile) strcpy(text, FAIL "missing operand\n");
            else {
                int fd = vfs->open(file, O_CREAT);
                if (fd>=0){
                    vfs->close(fd);
                    sprintf(text, SUCCESS "create file %s\n", file);
                } else {
                    sprintf(text, FAIL "cannot create file %s\n", file);
                }
            }
        } else if (strncmp(line, "cat", 3)==0) {
            getname(3);
            if (nofile) strcpy(text, FAIL "missing operand\n");
            else {
                int fd = vfs->open(file, O_RD);
                if (fd>=0){
                    if (FILE(fd)->inode->type!=NF){
                        sprintf(text, FAIL "Is a directory: %s\n", file);
                    } else {
                        memset(text, 0, 1024);;
                        vfs->read(fd, text, 1024);
                        vfs->close(fd);
                    }
                } else {
                    sprintf(text, FAIL "no such file or directory %s\n", file);
                }
            }
        } else if (strncmp(line, "link", 4)==0){
            gettwoname(4);
            if (nofile) strcpy(text, FAIL "missing operand\n");
            else {
                int ret = vfs->link(file, file2);
                if (ret == -1){
                    sprintf(text, FAIL "cannot create link to file %s\n", file);
                } else {
                    sprintf(text, SUCCESS "create link %s -> %s\n", file2, file);
                }
            }
        } else if (strncmp(line, "open", 4)==0){
            getname(4);
            if (nofile) strcpy(text, FAIL "missing operand\n");
            else {
                int fd = vfs->open(file, 0);
                sprintf(text, SUCCESS "open file %s -> fd=%d\n", file, fd);
            }
        } else if (strncmp(line, "close", 5)==0){
            int len = 5;
            if (line[len]==' '){
                strcpy(file, line+len+1);
            }
            int fd = my_atoi(file);
            vfs->close(fd);
            sprintf(text, SUCCESS "close file with fd=%d\n", fd);
        } else if (strncmp(line, "write", 5)==0){
            int len = 5;
            if (line[len]==' '){
                int slen;
                for (slen = len+1;line[slen]!='\0';slen++){
                    if (line[slen]==' ') break;
                }
                strncpy(file, line+len+1, slen-len-1);
                strcpy(file2, line+slen+1);
                modify(file2);
            }
            int fd = my_atoi(file);
            vfs->write(fd, file2, strlen(file2));
            sprintf(text, SUCCESS "write to file with fd=%d\n", fd);
        } else if (strncmp(line, "read", 4)==0){
            int len = 4;
            if (line[len]==' '){
                strcpy(file, line+len+1);
            }
            int fd = my_atoi(file);
            vfs->read(fd, text, 1024);
        } else if (strncmp(line, "lkset", 5)==0){
            int len = 5;
            if (line[len]==' '){
                int slen;
                for (slen = len+1;line[slen]!='\0';slen++){
                    if (line[slen]==' ') break;
                }
                strncpy(file, line+len+1, slen-len-1);
                strcpy(file2, line+slen+1);
            }
            int fd = my_atoi(file);
            int offset = my_atoi(file2);
            off_t of = vfs->lseek(fd, offset, S_SET);
            sprintf(text, "set offset to %d of file with fd=%d\n", of, fd);

        } else if (strncmp(line, "lkcur", 5)==0){

        } else if (strncmp(line, "lkend", 5)==0){

        } else {
            sprintf(text, FAIL "command not found %s\n", line);
        }

        tty_write(tty, 0, text, strlen(text));
    }
}