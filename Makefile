CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
PREPROCESSED_SRCS=$(SRCS:.c=.i)
OBJS=$(SRCS:.c=.o)
TEST_SRCS=$(wildcard test/*.c)
TESTS=$(TEST_SRCS:.c=.out)
RM=rm -rf

all: 9cc stage2/9cc stage3/9cc

test-all: test test-stage2 diff-stage23 err-test

9cc: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

$(PREPROCESSED_SRCS): %.i: %.c
	$(CC) -o $@ -U__GNUC__ -E -P -C $^

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

stage2/%.o: %.i 9cc
	@mkdir -p stage2
	./9cc $< > stage2/$*.s
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

stage3/%.o: %.i stage2/9cc
	@mkdir -p stage3
	./stage2/9cc $< > stage3/$*.s
	$(CC) -c -o $@ stage3/$*.s -g

stage3/9cc: $(OBJS:%=stage3/%)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

diff-stage23: stage3/9cc
	diff stage2/9cc $^
	@echo "OK"; echo

err-test: 9cc
	test/err_test.sh

clean:
	$(RM) 9cc $(PREPROCESSED_SRCS) *.o *~ test/*.s test/*.out stage2 stage3

.PHONY: all test-all test test-stage2 diff-stage23 err-test clean
