#ifndef INC_9CC_TEST_H
#define INC_9CC_TEST_H

#define ASSERT(x, y) assert(x, y, #y)

void assert(int expected, int actual, char *code);
int printf();

#endif
