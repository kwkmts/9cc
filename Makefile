CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
OBJS=$(SRCS: .c=.o)

9cc: $(OBJS)
	$(CC) -o 9cc $(OBJS) $(CFLAGS) $(LDFLAGS)

$(OBJS): 9cc.h

test: 9cc
	$(CC) -E -P -C test/test.c -o- | ./9cc - > test/test.s
	$(CC) test/test.s -xc test/common -o test/test -g
	./test/test

clean:
	rm -f 9cc *.o *~ tmp* test/test test/test.s

.PHONY: test clean
