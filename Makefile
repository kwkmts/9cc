CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
OBJS=$(SRCS: .c=.o)

9cc: $(OBJS)
	$(CC) -o 9cc $(OBJS) $(LDFLAGS)

$(OBJS): 9cc.h

debug: $(OBJS)
	$(CC) -o 9cc $(OBJS) $(LDFLAGS) -g

test: 9cc
	./test.sh

clean:
	rm -f 9cc *.o *~ tmp* *.core core

.PHONY: test clean debug
