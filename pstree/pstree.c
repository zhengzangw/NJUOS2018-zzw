#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
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
            printf("pstree 1.0\n Copyright (C) 2019 Zheng Zangwei");
            return 0;
          case 'n':
            issort = true;
            break;
          case 'p':
            showpid = true
      }
  }
  return 0;
}
