#pragma once

// This is a minimal stdarg.h for a freestanding environment using GCC or Clang.
// It relies on compiler-specific built-ins to handle variable arguments.

#ifndef _STDARG_H
#define _STDARG_H

typedef __builtin_va_list va_list;

#define va_start(v, l)  __builtin_va_start(v, l)
#define va_end(v)       __builtin_va_end(v)
#define va_arg(v, l)    __builtin_va_arg(v, l)

#endif // _STDARG_H