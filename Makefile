CFLAGS= -Wall
OBJS = mlisp.o parse.o debug.o

mlisp:  $(OBJS)
	$(CC) -g -o $@ $(OBJS)

$(OBJS): mlisp.h

.PHONY: clean cleanobj test

clean: cleanobj
	rm mlisp

cleanobj:
	rm -f *.o

test: mlisp
	@./test.sh

minitest: mlisp
	@./minitest.sh

all: mlisp clean mlisp
