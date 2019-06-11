#ifndef __UTIL_H__
#define __UTIL_H__

void strip(char *);
char *rootdir(const char* path);
char *filename(const char* path);
char *alldir(const char *path);
char *postname(const char *path);

int isdigit(int);
int atoi(char *);
#endif
