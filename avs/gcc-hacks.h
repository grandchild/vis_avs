#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER

// Special case for a GCC warning, see below. No effect with MSVC
#define FORCE_FUNCTION_CAST(x) (x)

#elif defined(__GNUC__)

// GCCs -Wcast-function-type is a bit overzealous when casting dynamic library function
// pointers (whose signatures _will_ be mismatched most of the time). The warning can be
// suppressed by first casting the function pointer to the special case void(*)(). This
// macro makes this a bit less cryptic and explicit, while still keeping the warning
// enabled for the cases where it might actually catch something.
#define FORCE_FUNCTION_CAST(x) (x)(void (*)())

// These are built-in in MSVC
int min(int a, int b);
int max(int a, int b);

#endif  // compilers

#ifdef __cplusplus
}
#endif
