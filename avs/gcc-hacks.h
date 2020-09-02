#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Naked functions are emitted without prolog or epilog (e.g. for compiler
// construction). GCC has no naked attribute for __declspec, but an
// __attribute__.
#ifdef _MSC_VER

#define NAKED __declspec(naked)

#else  // _MSC_VER

#define NAKED __attribute__((naked))

int min(int a, int b);
int max(int a, int b);

#endif  // _MSC_VER

#ifdef __cplusplus
}
#endif
