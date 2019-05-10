#include <stdio.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include <stdbool.h>
#include <locale.h>

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
  uint8_t sector_per_cluster;
  uint16_t num_of_res_sector;
  uint8_t num_of_fat;
  uint16_t zero_0;
  uint16_t zero_1;
  uint8_t material_type;
  uint16_t zero_2;
  uint16_t sector_per_tract;
  uint16_t num_of_head;
  uint32_t num_of_hidden_sector;
  uint32_t num_of_fs_sector;
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

struct LFILE {
  uint8_t serial;
  wchar_t low_name[5];
  uint8_t flag;
  uint8_t res;
  uint8_t checksum;
  wchar_t middle_name[6];
  uint16_t cluster;
  wchar_t high_name[2];
}__attribute__((packed));
typedef struct LFILE lfile_t;

bool isbmp(char *ptr){
    if (ptr[0]!='b'&&ptr[0]!='B') return false;
    if (ptr[1]!='m'&&ptr[1]!='M') return false;
    if (ptr[2]!='p'&&ptr[2]!='P') return false;
    return true;
}

#define BYTE(i) (*((uint8_t *) ptr + i))
#define LOGBYTE(i) printf("%02x\n", BYTE(i));
int main(int argc, char *argv[]) {
  char *img_ptr = mmap_open(argv[1]);
  char *end_ptr = img_ptr + size;
  dbr_t *dbr = malloc(sizeof(dbr_t));
  memcpy(dbr, img_ptr, sizeof(dbr_t));
  //int cluster_size = dbr->byte_per_sector * dbr->sector_per_cluster;
  printf("%u %u", sizeof(sfile_t), sizeof(lfile_t));

  char *data_ptr = img_ptr + dbr->byte_per_sector * (dbr->num_of_res_sector + dbr->num_of_fat * dbr->num_of_fat_sector);

  sfile_t *tmp = malloc(sizeof(sfile_t));
  int cnt_file = 0;
  for (char *ptr=data_ptr; ptr<=end_ptr; ptr+=32){
    memcpy(tmp, ptr, sizeof(sfile_t));
    if (isbmp(tmp->ext)) printf("FILE %d: %s\n", cnt_file++, tmp->name);
    lfile_t *l_ptr = (lfile_t *)(ptr - sizeof(lfile_t));
    while (l_ptr->flag == 0xF) {
        printf("%ls", l_ptr->low_name);
        printf("%ls", l_ptr->middle_name);
        printf("%ls\n", l_ptr->high_name);
        l_ptr --;
    }
  }

  mmap_close(img_ptr);
  return 0;
}
