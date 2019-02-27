#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <dirent.h>
typedef int bool;
#define true 1
#define false 0

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
  printf("%d%d", issort, showpid);

  DIR *dir;
  struct dirent *ent;
  if ((dir = opendir("/proc/"))!=NULL){
      while ((ent = readdir(dir))!=NULL){
        printf("%s\n", ent->d_name);
      }
      close(dir);
  } else {
    perror("");
    return -1;
  }
  return 0;
}
