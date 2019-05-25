#ifndef __VFS_H__
#define __VFS_H__

#include <common.h>

typedef struct filesystem {
  fsops_t *ops;
  device_t *dev;
} filesystem_t;

typedef struct fsops {
  void (*init)(filesystem_t *fs, const char *name, device_t *dev);
  inode_t *(*lookup)(filesystem_t *fs, const char *name, int flags);
  int (*close)(inode_t *inode);
} fsops_t;

typedef struct inodeops {
  int (*open)(file_t *file, int flags);
  int (*close)(file_t *file);
  ssize_t (*read)(file_t *file, char *buf, size_t size);
  ssize_t (*write)(file_t *file, const char *buf, size_t size);
  off_t (*lseek)(file_t *file, off_t offset, int whence);
  int (*mkdir)(const char *name);
  int (*rmdir)(const char *name);
  int (*link)(const char *name, inode_t *inode);
  int (*unlink)(const char *name);
} inodeops_t;

typedef struct inode {
  int refcnt;
  void *ptr;
  filesystem_t *fs;
  inodeops_t *ops;
} inode_t;

typedef struct file {
  int refcnt;
  inode_t *inode;
  uint64_t offset;
} file_t;

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
} MODULE(vfs);

#endif
