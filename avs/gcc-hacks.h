#pragma once

// Naked functions are emitted without prolog or epilog (e.g. for compiler
// construction). GCC has no naked attribute for __declspec, but an
// __attribute__.
#ifdef _MSC_VER
#define NAKED __declspec(naked)
#else
#define NAKED __attribute__((naked))
#endif
