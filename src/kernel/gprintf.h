#pragma once

#include <stdarg.h>

typedef void (*gprintf_handler_t)(void* context, char c);

void gprintf(gprintf_handler_t handler, void* context, const char* fmt, va_list args);