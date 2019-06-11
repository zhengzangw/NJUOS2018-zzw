#include <util.h>
#include <common.h>

void strip(char *tmp){
    int len = strlen(tmp);
    if (len!=1 && tmp[len-1]=='/'){
        tmp[len-1] = '\0';
    }
}

//path = usr/bin/zsh -> usr
//path = usr -> usr
char *rootdir(const char* path){
    if (!path || strlen(path) ==0) return NULL;
    char *ret = balloc(strlen(path)+1);
    strcpy(ret, path);
    for (int i=0;i<strlen(path);++i){
        if (path[i] == '/'){
            strncpy(ret, path, i);
            ret[i] = '\0';
            break;
        }
    }
    //Log("path = %s, ret = %s", path, ret);
    return ret;
}

//path = usr/bin/zsh -> zsh
//path = usr -> usr
char *filename(const char* path){
    if (!path || strlen(path) ==0) return NULL;
    char *ret = balloc(strlen(path)+1);
    strcpy(ret, path);
    for (int i=strlen(path);i>0;--i){
        if (path[i] == '/'){
            strncpy(ret, path+i+1, strlen(path+i+1));
            ret[strlen(path+i+1)] = '\0';
            break;
        }
    }
    //Log("path = %s, ret = %s", path, ret);
    return ret;
}

//path = usr/bin/zsh -> usr/bin
//path = usr -> NULL
char *alldir(const char *path){
    if (!path || strlen(path) ==0) return NULL;
    char *ret = balloc(strlen(path));
    for (int i=strlen(path);i>=0;--i){
        if (path[i] == '/'){
            strncpy(ret, path, i);
            ret[i] = '\0';
            break;
        }
    }
    //Log("path = %s, ret = %s", path, ret);
    return ret;
}

//path = usr/bin/zsh -> bin/zsh
//path = usr -> NULL
char *postname(const char *path){
    if (!path || strlen(path) ==0) return NULL;
    char *ret = balloc(strlen(path));
    for (int i=strlen(path);i>=0;--i){
        if (path[i] == '/'){
            strncpy(ret, path+i+1, strlen(path+i+1));
            ret[strlen(path+i+1)] = '\0';
            break;
        }
    }
    //Log("path = %s, ret = %s", path, ret);
    return ret;
}

int isdigit(int ch){return (ch>='0')&&(ch<='9');}
int isnum(const char *s){
    for (int i=0;i<strlen(s);++i){
        if (!isdigit(s)) return 0;
    }
    return 1;
}
int atoi(const char *s){
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
