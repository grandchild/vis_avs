#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Naked functions are emitted without prolog or epilog (e.g. for compiler
// construction). GCC has no naked attribute for __declspec, but an
// __attribute__.
#ifdef _MSC_VER

#define NAKED __declspec(naked)

// Special case for a GCC warning, see below. No effect with MSVC
#define FORCE_FUNCTION_CAST(x) (x)

#else  // _MSC_VER

#define NAKED __attribute__((naked))

// GCCs -Wcast-function-type is a bit overzealous when casting dynamic library function
// pointers (whose signatures _will_ be mismatched most of the time). The warning can be
// suppressed by first casting the function pointer to the special case void(*)(). This
// macro makes this a bit less cryptic and explicit, while still keeping the warning
// enabled for the cases where it might actually catch something.
#define FORCE_FUNCTION_CAST(x) (x)(void (*)())

int min(int a, int b);
int max(int a, int b);

#endif  // _MSC_VER

#ifdef __cplusplus
}
#endif
