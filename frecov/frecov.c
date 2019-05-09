#include <stdio.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>

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
  uint8_t jmp_intr[3];
  char version[8];
  uint16_t byte_per_sector;
  uint8_t cluster_per_sector;
  uint16_t num_of_res_sector;
  uint8_t num_of_fat;
  uint16_t zero_0;
  uint16_t zero_1;
  uint8_t material_type;
  uint16_t zero_2;
  uint16_t sector_per_tract;
  uint16_t num_of_head;
  uint32_t num_of_hidden_sector;
  uint32_t total_sector;
  uint32_t num_of_fat_sector;
  uint16_t label;
  uint16_t fs_version;
  uint32_t root_dir;
} __attribute__((packed));
typedef struct DBR dbr_t;

struct SFILE {
  char name[8];
  char ext[3];
  uint8_t attr;
  uint8_t res;
  uint8_t c_ms;
  uint16_t c_time;
  uint16_t c_date;
  uint16_t v_date;
  uint16_t high_cluster;
  uint16_t m_time;
  uint16_t m_date;
  uint16_t low_cluster;
  uint32_t size;
}__attribute__((packed));
typedef struct SFILE sfile_t;

#define BYTE(i) (*((uint8_t *) ptr + i))
#define LOGBYTE(i) printf("%02x\n", BYTE(i));
int main(int argc, char *argv[]) {
  char *img_ptr = mmap_open(argv[1]);
  dbr_t *dbr = malloc(sizeof(dbr_t));
  memcpy(dbr, img_ptr, sizeof(dbr_t));

  char *data_ptr = img_ptr + 1ll * dbr->byte_per_sector *(dbr->num_of_res_sector + dbr->num_of_fat * dbr->num_of_fat_sector);

  sfile_t *root_dir = malloc(sizeof(sfile_t));
  memcpy(root_dir, data_ptr, sizeof(sfile_t));
  printf("%s", root_dir->name);
  printf("%d", root_dir->c_date);
  printf("%d", root_dir->size);

  mmap_close(img_ptr);
  return 0;
}
