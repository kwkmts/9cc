CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
PREPROCESSED_SRCS=$(SRCS:.c=.i)
OBJS=$(SRCS:.c=.o)
TEST_SRCS=$(wildcard test/*.c)
TESTS=$(TEST_SRCS:.c=.out)
RM=rm -rf

all: 9cc stage2/9cc stage3/9cc

test-all: test test-stage2 cmp-stage23

err-test-all: err-test err-test-stage2

9cc: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

$(PREPROCESSED_SRCS): %.i: %.c
	$(CC) -o $@ -U__GNUC__ -E -P -C $^

$(OBJS): 9cc.h

test/macro.out: test/macro.c 9cc
	./9cc $< > test/macro.s
	$(CC) test/macro.s -xc test/common -o $@ -g

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

stage2/test/macro.out: test/macro.c stage2/9cc
	@mkdir -p stage2/test
	./stage2/9cc $< > stage2/test/macro.s
	$(CC) stage2/test/macro.s -xc test/common -o $@ -g

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

cmp-stage23: stage3/9cc
	strip -s stage2/9cc
	strip -s $^
	cmp stage2/9cc $^
	@echo "OK"; echo

err-test: 9cc
	test/err_test.sh ./$^

err-test-stage2: stage2/9cc
	test/err_test.sh ./$^

hashmap-test:
	cc -e hashmap_test -o hashmap_test *.c && ./hashmap_test
	@echo "OK"; echo

clean:
	$(RM) 9cc hashmap_test $(PREPROCESSED_SRCS) *.o *~ test/*.s test/*.out stage2 stage3

.PHONY: all test-all test test-stage2 cmp-stage23 err-test clean
