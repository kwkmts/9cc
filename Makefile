CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)
TEST_SRCS=$(wildcard test/*.c)
TESTS=$(TEST_SRCS:.c=.out)
RM=rm -rf

9cc: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

$(OBJS): 9cc.h

test/%.out: test/%.c 9cc
	$(CC) -o- -E -P -C $< | ./9cc - > test/$*.s
	$(CC) test/$*.s -xc test/common -o $@ -g

test: $(TESTS)
	@for t in $^; do \
		echo "== $$(basename "$$t" .out) =="; \
		./$$t || exit 1; \
		echo; echo "OK"; echo; \
	done

stage2/%.o: %.c 9cc
	@mkdir -p stage2
	$(CC) -o- -U__GNUC__ -E -P -C $< | ./9cc - > stage2/$*.s
	$(CC) -c -o $@ stage2/$*.s -g

stage2/9cc: $(OBJS:%=stage2/%)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

stage2/test/%.out: test/%.c stage2/9cc
	@mkdir -p stage2/test
	$(CC) -o- -E -P -C $< | ./stage2/9cc - > stage2/test/$*.s
	$(CC) stage2/test/$*.s -xc test/common -o $@ -g

test-stage2: $(TESTS:test/%=stage2/test/%)
	@for t in $^; do \
		echo "== $$(basename "$$t" .out) =="; \
		./$$t || exit 1; \
		echo; echo "OK"; echo; \
	done

err-test: 9cc
	test/err_test.sh

clean:
	$(RM) 9cc *.o *~ test/*.s test/*.out stage2

.PHONY: test test-stage2 err-test clean
