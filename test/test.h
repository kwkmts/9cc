#ifndef INC_9CC_TEST_H
#define INC_9CC_TEST_H

#define ASSERT(x, y) assert(x, y, #y)

void assert(int expected, int actual, char *code);
int printf(const char *, ...);
int sprintf(char *, const char *, ...);
int strcmp(const char *, const char *);
int vsprintf(char *, const char *, __builtin_va_list);

extern int ext3;

#endif
