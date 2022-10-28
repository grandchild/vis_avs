#pragma once

// "Signed" or "small" sizeof operators. When sizeof x is known to fit into a specific
// signed integer, i.e. ssizeofN(x) => sizeof(x) <= 2^(N-1). This suppresses signed /
// unsigned comparison warnings, without turning everything on the other side into
// unsigned types, which is not always desired or possible.
// If the size assumption is violated the value will overflow and you will get negative
// sizes.
#define ssizeof8(x)  ((int8_t)sizeof(x))
#define ssizeof32(x) ((int32_t)sizeof(x))
#define ssizeof64(x) ((int64_t)sizeof(x))
