NAME := $(shell basename $(PWD))
export TASK := M6
all: $(NAME)-64.so $(NAME)-32.so
CFLAGS += -U_FORTIFY_SOURCE

include ../Makefile
include ../Makefile.lab

test: kvdb.c test.c
	make
	gcc -pthread -L. -Wl,-rpath=. -lkvdb-64 test.c -o test
