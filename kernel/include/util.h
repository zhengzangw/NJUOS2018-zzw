#ifndef __UTIL_H__
#define __UTIL_H__

int split(const char *path, char **pre, char **post);
int split2(const char *path, char **pre, char **post);

void strip(char *);
char *rootdir(const char* path);
char *filename(const char* path);
char *alldir(const char *path);
char *postname(const char *path);
#endif