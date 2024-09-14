#ifndef _STDARG_H
#define _STDARG_H

typedef __builtin_va_list __gnuc_va_list;
typedef __gnuc_va_list va_list;

#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_end(ap) __builtin_va_end(ap)
#define va_arg(ap, ty) __builtin_va_arg(ap, ty)
#define va_copy(dst, src) __builtin_va_copy(dst, src)

#endif
