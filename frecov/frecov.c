#include <stdio.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>

int size;
void *mmap_open(char *name){
  int fd = open(name, O_RDONLY);
  assert(fd!=-1);

  struct stat file_stat;
  fstat(fd, &file_stat);
  size = file_stat.st_size;

  void *start_fp;
  start_fp = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  assert(start_fp != MAP_FAILED);

  return start_fp;
}

void mmap_close(void *fp){
  munmap(fp, size);
}

struct DBR {

};
typedef struct DBR DBR_t;

#define BYTE(i) (*((uint8_t *) ptr + i))
#define LOGBYTE(i) printf("%02x\n", BYTE(i));
int main(int argc, char *argv[]) {
  void *ptr = mmap_open(argv[1]);
 LOGBYTE(0);
  mmap_close(ptr);
  return 0;
}
