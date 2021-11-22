#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void lock_t;

/*
 * OS specifics
 */
#ifdef __WIN32

#elif defined __linux__

#include <stdlib.h>

// 1 drive letter + 1 colon + 1 backslash + 256 path + 1 terminating null
#define MAX_PATH 260

// typedef pthread_mutex_t lock_t;

#endif  // OS specifics

/*
 * Compiler specifics
 */
#ifdef __GNUC__

/*
GCCs -Wcast-function-type is a bit overzealous when casting dynamic library function
pointers (whose signatures _will_ be mismatched most of the time). The warning can be
suppressed by first casting the function pointer to the special case void(*)(). This
macro makes this a bit less cryptic and explicit, while still keeping the warning
enabled for the cases where it might actually catch something.
*/
#define FORCE_FUNCTION_CAST(x) (x)(void (*)())

// These are builtins in MSVC
int min(int a, int b);
int max(int a, int b);

#else

// A no-op here, see above.
#define FORCE_FUNCTION_CAST(x) (x)

#endif  // Compiler specifics

/*
Return the number of milliseconds since the system was booted. Since it's a 32bit value,
it will roll over roughly every 50 days. The function will return 0 on error. If the
rollover is hit precisely it will return 1 instead of 0 (not really a loss of precision,
because the system timer itself isn't that precise).
*/
uint32_t millis_since_boot();

lock_t* lock_init();
void lock(lock_t* lock_obj);
bool lock_try(lock_t* lock_obj);
void lock_unlock(lock_t* lock_obj);
void lock_destroy(lock_t* lock_obj);

#ifdef __cplusplus
}
#endif
