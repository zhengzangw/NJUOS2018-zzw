#include <stdio.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <malloc.h>
#include <stdbool.h>
#include <locale.h>
#include <wchar.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

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
  unsigned short low_name[5];
  uint8_t flag;
  uint8_t res;
  uint8_t checksum;
  unsigned short middle_name[6];
  uint16_t cluster;
  unsigned short high_name[2];
}__attribute__((packed));
typedef struct LFILE lfile_t;

struct bmp_header {
  char id[2];
  uint32_t size;
  uint32_t res;
  uint32_t offset;
}__attribute__((packed));
typedef struct bmp_header bmp_t;

struct Entry {
  wchar_t name[128];
  char sha1sum[100];
  uint32_t size;
};
typedef struct Entry entry_t;
entry_t ans[8192];

int cmp(const void *a, const void *b){
    return (((entry_t *)a)->size-((entry_t *)b)->size);
}

bool isbmp(char *ptr){
    if (ptr[0]!='b'&&ptr[0]!='B') return false;
    if (ptr[1]!='m'&&ptr[1]!='M') return false;
    if (ptr[2]!='p'&&ptr[2]!='P') return false;
    return true;
}

bool iswbmp(wchar_t *ptr){
    int ppos;
    bool flag = false;
    for (ppos=0; ppos<wcslen(ptr); ++ppos){
        if (ptr[ppos]==L'.') {
            flag = true;
            break;
        }
    }

    if (flag) {
      ptr[ppos+1] = L'b';
      ptr[ppos+2] = L'm';
      ptr[ppos+3] = L'p';
    }
    return flag;
}

bool validbmp(bmp_t *bmp_ptr, int size){
    if (bmp_ptr->id[0]!='B'||bmp_ptr->id[1]!='M') return false;
    if (bmp_ptr->size != size) return false;
    return true;
}

void display(char *name, bmp_t *bmp_ptr){
    FILE *bmp = fopen(name, "wb");
    fwrite(bmp_ptr, bmp_ptr->size, 1, bmp);
    fclose(bmp);
}

#define BYTE(i) (*((uint8_t *) ptr + i))
#define LOGBYTE(i) printf("%02x\n", BYTE(i));
char *argv_sha1sum[] = {"/usr/bin/strace"};
int main(int argc, char *argv[], char *env[]) {
  char *img_ptr = mmap_open(argv[1]);
  char *end_ptr = img_ptr + size;
  dbr_t *dbr = malloc(sizeof(dbr_t));
  memcpy(dbr, img_ptr, sizeof(dbr_t));
  int cluster_size = dbr->byte_per_sector * dbr->sector_per_cluster;

  char *data_ptr = img_ptr + dbr->byte_per_sector * (dbr->num_of_res_sector + dbr->num_of_fat * dbr->num_of_fat_sector);

  int cnt_file = 0;
  wchar_t name[128];
  for (sfile_t *ptr=(sfile_t *)data_ptr; ptr<=(sfile_t *)end_ptr; ptr++){
    if (isbmp(ptr->ext)) { //Judge is a bmp inode
        lfile_t *l_ptr = (lfile_t *)ptr - 1;
        bzero(name, sizeof(name));
        int base = 0;
        while (l_ptr->flag == 0xF && l_ptr->cluster==0) {
            for (int i=0;i<5;++i) name[base+i] = l_ptr->low_name[i];
            for (int i=0;i<6;++i) name[base+i+5] = l_ptr->middle_name[i];
            for (int i=0;i<2;++i) name[base+i+11] = l_ptr->high_name[i];
            base += 13;
            l_ptr --;
        }
        if (!iswbmp(name)||ptr->size==0||ptr->low_cluster==0) continue; //Inode loss
        uint32_t offset = (uint32_t)ptr->high_cluster<<16|ptr->low_cluster;
        uint32_t addr = (offset-2)*cluster_size;
        bmp_t *bmp_ptr = (bmp_t *)(data_ptr+addr);
        if (!validbmp(bmp_ptr, ptr->size)) continue;

        //int status;
        int fl_in[2], fl_out[2];
        pipe(fl_in); pipe(fl_out);
        int pid = fork();
        if (pid == 0){
            dup2(fl_in[0], STDIN_FILENO);
            dup2(fl_out[1], STDOUT_FILENO);
            execve("/usr/bin/sha1sum", argv_sha1sum, env);
        } else {
            FILE *output = fdopen(fl_in[1], "wb");
            FILE *input = fdopen(fl_out[0], "r");
            fwrite(bmp_ptr, bmp_ptr->size, 1, output);
            //fsync(fl_out[0]);
            fclose(output);
            //wait(&status);
            fscanf(input, " %s", ans[cnt_file].sha1sum);
            fclose(input);
        }

        wcscpy(ans[cnt_file].name, name);
        ans[cnt_file].size = ptr->size;
        cnt_file++;
    }
  }

  qsort(ans, cnt_file, sizeof(entry_t), cmp);
  for (int i=0;i<cnt_file;++i)
    printf("Name %ls sha1sum %s", ans[i].name, ans[i].sha1sum);

  mmap_close(img_ptr);
  return 0;
}
