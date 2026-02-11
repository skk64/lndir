

#ifndef debug_printf
#ifdef DEBUG
#define debug_printf(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__);
#else
#define debug_printf(fmt, ...)
#endif
#endif
