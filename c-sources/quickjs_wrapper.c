#include <stddef.h>

/*
 * QuickJS performs left shifts on signed integers (e.g. 168 << 24) which is
 * technically undefined behavior in C. The Zig/Clang compiler inserts
 * UndefinedBehaviorSanitizer (UBSan) checks that cause a runtime panic when
 * this occurs.
 *
 * We use these pragmas to disable the "shift" sanitizer for all functions
 * included from quickjs.c, allowing the code to run as intended without crashing.
 */
#ifdef __clang__
#pragma clang attribute push (__attribute__((no_sanitize("shift"))), apply_to=function)
#endif

#include "../quickjs/quickjs.c"

#ifdef __clang__
#pragma clang attribute pop
#endif

