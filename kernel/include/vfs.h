#ifndef __VFS_H__
#define __VFS_H__

#include <common.h>

/* ==== VFS ==== */

typedef struct inodeops inodeops_t;
typedef struct filesystem filesystem_t;
typedef struct fsops fsops_t;
typedef struct inode {
  filesystem_t *fs;
  inodeops_t *ops;
  void *fs_inode;
  //Public
  uint16_t permission;
  uint32_t size;
  uint32_t id;
  uint32_t type;
  uint32_t dir_len;
  uint32_t link_num;
} inode_t;

struct filesystem {
  fsops_t *ops;
  device_t *dev;
};

struct fsops {
  void (*init)(filesystem_t *fs, const char *name, device_t *dev);
  inode_t *(*lookup)(filesystem_t *fs, const char *name, int flags);
  int (*close)(inode_t *inode);
  //Additional
  int (*mkdir)(filesystem_t *fs, const char *name);
  int (*rmdir)(filesystem_t *fs, const char *name);
  int (*unlink)(filesystem_t *fs, const char *name);
  int (*create)(filesystem_t *fs, const char *name);
};

typedef struct file {
  inode_t *inode;
  uint64_t offset;
} file_t;

struct inodeops {
  int (*open)(file_t *file, int flags);
  int (*close)(file_t *file);
  ssize_t (*read)(file_t *file, char *buf, size_t size);
  ssize_t (*write)(file_t *file, const char *buf, size_t size);
  off_t (*lseek)(file_t *file, off_t offset, int whence);
  int (*link)(file_t *file, const char*name);
  //Additional
};

typedef struct mountpoint {
  const char *path;
  filesystem_t *fs;
  int exists;
} mountpoint_t;
#define MAXMP 64

typedef struct {
  void (*init)();
  int (*access)(const char *path, int mode);
  int (*mount)(const char *path, filesystem_t *fs);
  int (*unmount)(const char *path);
  int (*mkdir)(const char *path);
  int (*rmdir)(const char *path);
  int (*link)(const char *oldpath, const char *newpath);
  int (*unlink)(const char *path);
  int (*open)(const char *path, int flags);
  ssize_t (*read)(int fd, void *buf, size_t nbyte);
  ssize_t (*write)(int fd, void *buf, size_t nbyte);
  off_t (*lseek)(int fd, off_t offset, int whence);
  int (*close)(int fd);
  //Additional
} MODULE(vfs);

/* ===== MACRO ===== */
#define FILE(fd) cputask[_cpu()]->flides[fd]
//TYPE
enum TYPE {NF, DR, XX, MP};
//access
#define R_OK 1
#define W_OK 2
#define X_OK 8
#define F_OK 16
//open
#define O_RD 1
#define O_WR 2
#define O_CREAT  4
#define O_TRUNC  8
//lseek
#define S_SET 0
#define S_CUR 1
#define S_END 2

/* ====== ext2 ======= */
extern fsops_t ext2_ops;
typedef struct ext2_inode ext2_inode_t;
extern inodeops_t ext2_inodeops;

#define BLOCK_BYTES (1<<10)
#define INODE_BYTES (1<<7)
#define BLOCK(x) ((x)*BLOCK_BYTES)
#define TABLE(i) (BLOCK(ITABLE)+(i)*INODE_BYTES)
#define DATA(i) (BLOCK(DATA_B)+(i)*BLOCK_BYTES)
#define IMAP 0
#define DMAP 1
#define ITABLE 2
#define ITABLE_NUM 10
#define DATA_B (ITABLE+ITABLE_NUM)
#define OFFSET_BLOCK(offset) (inode->link[(offset)/BLOCK_BYTES])
#define OFFSET_REMAIN(offset) ((offset)%BLOCK_BYTES)

//Inode
ext2_inode_t* ext2_inode_create(device_t *dev, uint8_t type, uint8_t per);
void ext2_inode_remove(device_t *, ext2_inode_t*);
ext2_inode_t* ext2_inode_lookup(device_t *dev, const char *name);

int ext2_dir_search(device_t *, ext2_inode_t*, const char*);
//Log
#define LOG_NUM 4
#define LOGBLOCK() \
    for (int i=0;i<DATA_B+LOG_NUM;++i)\
        LogBlock(dev, i)
void LogBlock(device_t*, int);

#endif
