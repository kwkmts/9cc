CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
PREPROCESSED_SRCS=$(SRCS:.c=.i)
OBJS=$(SRCS:.c=.o)
TEST_SRCS=$(wildcard test/*.c)
TESTS=$(TEST_SRCS:.c=.out)
RM=rm -rf

all: 9cc

test-all: test test-stage2 cmp-stage23

err-test-all: err-test err-test-stage2

9cc: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

$(OBJS): 9cc.h

test/%.out: test/%.c 9cc
	./9cc $< -o test/$*.s
	$(CC) test/$*.s -xc test/common -o $@ -g

test: $(TESTS)
	@for t in $^; do \
		echo "== $$(basename "$$t" .out) =="; \
		./$$t || exit 1; \
		echo; echo "OK"; echo; \
	done

stage2/%.o: 9cc
	@mkdir -p stage2
	./9cc -o stage2/$*.s $*.c
	$(CC) -c -o $@ stage2/$*.s -g

9cc.stage2: $(OBJS:%=stage2/%)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

stage2/test/%.out: test/%.c 9cc.stage2
	@mkdir -p stage2/test
	./9cc.stage2 $< -o stage2/test/$*.s
	$(CC) stage2/test/$*.s -xc test/common -o $@ -g

test-stage2: $(TESTS:test/%=stage2/test/%)
	@for t in $^; do \
		echo "== $$(basename "$$t" .out) =="; \
		./$$t || exit 1; \
		echo; echo "OK"; echo; \
	done

stage3/%.o: 9cc.stage2
	@mkdir -p stage3
	./9cc.stage2 -o stage3/$*.s $*.c
	$(CC) -c -o $@ stage3/$*.s -g

9cc.stage3: $(OBJS:%=stage3/%)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

cmp-stage23: 9cc.stage3
	strip -s 9cc.stage2
	strip -s $^
	cmp 9cc.stage2 $^
	@echo "OK"; echo

err-test: 9cc
	test/err_test.sh ./$^

err-test-stage2: 9cc.stage2
	test/err_test.sh ./$^

driver-test: 9cc
	test/driver_test.sh

clean:
	$(RM) 9cc 9cc.stage2 9cc.stage3 *.o *~ test/*.s test/*.out stage2 stage3

.PHONY: all test-all err-test-all test test-stage2 cmp-stage23 err-test err-test-stage2 driver-test clean
