#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

char buf[20000];
int main(int argc, char *argv[]) {

    while (true){
      printf(">> ");
      scanf("%s", buf);

      FILE *tmpfp = tmpfile();
      if (tmpfp==NULL){
          perror("tmpfile error");
          return 1;
      }
      fputs(buf, tmpfp);



      printf("  %s\n", buf);
    }
  return 0;
}
