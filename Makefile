CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)
TEST_SRCS=$(wildcard test/*.c)
TESTS=$(TEST_SRCS:.c=.out)

9cc: $(OBJS)
	$(CC) -o 9cc $(OBJS) $(CFLAGS) $(LDFLAGS)

$(OBJS): 9cc.h

test/%.out: test/%.c 9cc
	$(CC) -o- -E -P -C test/$*.c | ./9cc - > test/$*.s
	$(CC) test/$*.s -xc test/common -o $@ -g

test: $(TESTS)
	@for t in $^; do \
		echo "== $$(basename "$$t" .out) =="; \
		./$$t || exit 1; \
		echo; echo "OK"; echo; \
	done

err-test: 9cc
	test/err_test.sh

clean:
	rm -f 9cc *.o *~ test/*.s test/*.out

.PHONY: test err-test clean
