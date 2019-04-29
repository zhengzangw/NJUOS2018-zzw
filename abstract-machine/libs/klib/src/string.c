#include "klib.h"

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  const char *ptr;
	for (ptr=s; *ptr!='\0';++ptr);
  return ptr-s;
}

size_t strnlen(const char *s, size_t n){
  const char *ptr;
	for (ptr=s; *ptr!='\0'&&(ptr-s)<n;++ptr);
	return ptr-s;
}

char *strcpy(char* dst,const char* src) {
  char *ret = dst;
  while ((*dst++=*src++)!='\0');	

  return ret;
}

char* strncpy(char* dst, const char* src, size_t n) {
  char *ret = dst;
	while (n){
			if ((*dst=*src)!='\0') src++;
			dst++; n--;
	}
  return ret;
}

char* strcat(char* dst, const char* src) {
  char *ret = dst;
	while (*dst) dst++;
	while ((*dst++=*src++)!='\0');
  return ret;
}

int strcmp(const char* s1, const char* s2) {
  unsigned char c1,c2;
	while (1){
		c1 = *s1++;
		c2 = *s2++;
		if (c1!=c2) return c1<c2 ? -1:1;
		if (!c1) break;
	}
	return 0;
}

int strncmp(const char* s1, const char* s2, size_t n) {
  unsigned char c1,c2;
	while (n){
		c1 = *s1++;
		c2 = *s2++;
		if (c1!=c2) return c1<c2 ? -1:1;
		if (!c1) break;
		n--;
	}
	return 0;
}

void* memset(void* v,int c,size_t n) {
  char *ptr = v;
	while (n--) *ptr++=c;
  return v;
}

void* memcpy(void* out, const void* in, size_t n) {
  char *ptr = out;
	const char *s = in;
	while (n--) *ptr++=*s++;
  return out;
}

int memcmp(const void* s1, const void* s2, size_t n){
  const unsigned char *c1,*c2;
	int res = 0;

	for (c1 = s1, c2 = s2;n>0;++c1,++c2,n--)
		if ((res=*c1-*c2)!=0) break;
  return res;
}

#endif
