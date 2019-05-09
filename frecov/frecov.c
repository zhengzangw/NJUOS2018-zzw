#include <stdio.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char *argv[]) {
  int fd = open(argv[1], O_RDONLY);
  assert(fd!=-1);

  struct stat file_stat;
  fstat(fd, &file_stat);
  printf("size=%ld\n", file_stat.st_size);

  void *start_fp;
  start_fp = mmap(NULL, file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  assert(start_fp != MAP_FAILED);

  printf("%x\n", *(unsigned int *)start_fp);

  munmap(start_fp, file_stat.st_size);

  return 0;
}
