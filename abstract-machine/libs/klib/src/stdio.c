#include "klib.h"
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

#define ZEROPAD 1  // pad with zero
#define SIGN    2  // unsigned
#define PLUS    4  // plus
#define SPACE   8  // space
#define LEFT		16 // left aligned
#define SPECIAL 32 // 0x
#define LARGE		64 // ABCDEF

static inline int isdigit(int ch){return (ch>='0')&&(ch<='9');}
static inline int my_atoi(const char **s){
		int i=0;
		while (isdigit(**s))
				i = i*10+*((*s)++) - '0';
		(*s)--;
		return i;
}
static inline int div(long *n, unsigned base){
		int ret = ((unsigned long)*n)%(unsigned)base;
		*n = ((unsigned long)*n) / (unsigned) base;
		return ret;
}

static char *get_number(char *str, long num, int base, int size, int precision, int type){
		char c,sign,tmp[128];
		const char *digits;
		int i;

		if (type & LARGE)
				digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
		else
				digits = "0123456789abcdefghijklmnopqrstuvwxyz";
		if (type & LEFT)
				type &= ~ZEROPAD;
		if (base<2 || base > 36) assert(0);
		c = (type & ZEROPAD) ? '0' : ' ';
		sign = 0;
		if (type & SIGN){
				if (num < 0) {
						sign = '-';
						num = -num;
						size --;
				} else if (type & PLUS){
						sign = '+';
						size--;
				} else if (type & SPACE){
						sign = ' ';
						size--;
				}
		}
		if (type & SPECIAL) {
				if (base == 16)
						size -= 2;
				else if (base == 8)
						size --;
		}
		i = 0;
		if (num == 0)
		{
				tmp[i++] = '0';
		}
		else {
				while (num!=0){
						tmp[i++] = digits[div(&num,base)];
						//_putc(tmp[i-1]); _putc('\n');
				}
		}

		if (i>precision)
				precision = i;
		size -= precision;
		if (!(type & (ZEROPAD + LEFT)))
				while (size-->0)
						*str++ = ' ';
		if (sign)
				*str++ = sign;
		if (type & SPECIAL) {
				if (base == 8)
						*str++ = '0';
				else if (base == 16){
						*str++ = '0';
						*str++ = digits[33];
				}
		}
		if (!(type&LEFT))
				while (size-- > 0) *str++ = c;
		while (i<precision--)
				*str++ = '0';
		while (i-- > 0)
				*str++ = tmp[i];
		while (size-- > 0)
				*str++ = ' ';
		return str;
}

int printf(const char *fmt, ...) {
  char buf[1024];
	va_list args;
	int ret;

	va_start(args, fmt);
  ret = vsprintf(buf, fmt, args);
	va_end(args);

	uint32_t len = strlen(buf);
  for (int i=0;i<len;++i)
			_putc(buf[i]);

  return ret;
}

int vsprintf(char *out, const char *fmt, va_list args) {
	char *str;
	int flags;
	int field_width;
	int precision;
	int qualifier;
	int base;

	int len;
	const char *s;
	uint32_t num;

  for (str=out; *fmt; ++fmt){
			if (*fmt != '%') {
					*str++ = *fmt;
					continue;
			}

			flags = 0;
		  field_width = -1;
			precision = -1;
			qualifier = -1;
			base = 10;
			//_putc('O'); _putc('\n');
repeat:
			//_putc('I'); _putc('\n');
		  ++fmt;
			//_putc(*fmt); _putc('\n');
			if (*fmt==':') _halt(0);
			switch(*fmt){
					case '-':
							flags |= LEFT;
							goto repeat;
					case '+':
							flags |= PLUS;
							goto repeat;
					case ' ':
							flags |= SPACE;
							goto repeat;
					case '#':
							flags |= SPECIAL;
							goto repeat;
					case '0':
							flags |= ZEROPAD;
							goto repeat;
			}

			//_putc('D'); _putc('\n');
			if (isdigit(*fmt)){
					field_width = my_atoi(&fmt);
					goto repeat;
			}
			else if (*fmt == '*') {
					field_width = va_arg(args, int);
					if (field_width < 0){
							field_width = -field_width;
							flags |= LEFT;
					}
					goto repeat;
			}

			if (*fmt== '.') {
					++fmt;
					if (isdigit(*fmt))
							precision = my_atoi(&fmt);
				  else if (*fmt=='*') {
							precision = va_arg(args, int);
					}
					if (precision < 0)
							precision = 0;
					goto repeat;
			}

			if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L') {
					qualifier = *fmt;
					goto repeat;
			}

		  switch (*fmt) {
					case 'c':
							if (!(flags & LEFT))
									while (--field_width>0)
											*str++ = ' ';
							*str++ = (unsigned char) va_arg(args, int);
							while (--field_width>0)
									*str++ = ' ';
							continue;

					case 's':
						  s = va_arg(args, char *);
							len = strnlen(s, precision);
							if (!(flags & LEFT))
									while (len < field_width--)
											*str++ = ' ';
							for (int i=0; i<len; ++i)
									*str++ = *s++;
							while (len < field_width--)
									*str++ = ' ';
							continue;

					case 'p':
							base = 16;
							flags |= SPECIAL;
							if (field_width == -1) {
									field_width = 2 * sizeof(void *);
									flags |= ZEROPAD;
							}
							str = get_number(str,
											(unsigned long)va_arg(args, void *), base,
											field_width, precision, flags);
							continue;

					case 'n':
							if (qualifier == 'l'){
									long *ip = va_arg(args, long *);
									*ip = (str-out);
							} else {
									int *ip = va_arg(args, int *);
									*ip = (str-out);
							}
							continue;

				  case '%':
						  if (!(flags&LEFT))
									while (--field_width>0)
											*str++ = ' ';
						  *str++ = '%';
							while (--field_width>0)
									*str++ = ' ';
						  continue;

				  case 'o':
							base = 8;
							break;

				  case 'X':
							flags |= LARGE;
				  case 'x':
							base = 16;
							break;

				  case 'd':
					case 'i':
							flags |= SIGN;
				  case 'u':
							break;

				  default:
							assert(0);
			}
			if (qualifier == 'l')
					num = va_arg(args, unsigned long);
			else if (qualifier == 'h') {
					num = (unsigned short)va_arg(args, int);
					if (flags & SIGN)
							num = (short)num;
			} else if (flags & SIGN){
					num = va_arg(args, int);
			}
			else
					num = va_arg(args, unsigned int);

			str = get_number(str, num, base, field_width, precision, flags);

	}

  *str = '\0';

  return str-out;
}

int sprintf(char *out, const char *fmt, ...) {
  va_list args;
	va_start(args, fmt);
	int ret = vsprintf(out, fmt, args);
	va_end(args);
  return ret;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  assert(0);
  return 0;
}

#endif
