#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <dirent.h>
#include <string.h>

typedef int bool;
#define true 1
#define false 0

bool isnumber(char* s, int length){
    for (int i=0;i<length;++i){
        if (!(s[i]>='0'&&s[i]<='9')){
            return false;
        }
    }
    return true;
}

int main(int argc, char *argv[]) {
  // getopt
  bool issort=false, showpid=false;
  int opt;
  while ((opt = getopt(argc, argv, "Vnp"))!=-1){
      switch (opt) {
          case 'V':
            printf("pstree 1.0\nCopyright (C) 2019 Zheng Zangwei\n");
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

  DIR *dir;
  struct dirent *ent;
  if ((dir = opendir("/proc/"))!=NULL){
      while ((ent = readdir(dir))!=NULL){
        if (isnumber(ent->d_name, strlen(ent->d_name))){
          printf("%s\n", ent->d_name);
        }
      }
      closedir(dir);
  } else {
    perror("");
    return -1;
  }
  return 0;
}
