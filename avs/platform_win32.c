#include "platform.h"

#include <windows.h>
#include <commctrl.h>
#include <windowsx.h>

/* timers */

static int64_t _timer_highres_ticks_per_us = 0;

void _timer_us_init() {
    LARGE_INTEGER ticks_per_second = {.QuadPart = 0};
    if (!QueryPerformanceFrequency(&ticks_per_second)) {
        _timer_highres_ticks_per_us = 1;
    } else {
        _timer_highres_ticks_per_us = ticks_per_second.QuadPart / (1000 * 1000);
    }
}

uint64_t timer_ms() { return GetTickCount64(); }

uint64_t timer_us() {
    if (_timer_highres_ticks_per_us == 0) {
        _timer_us_init();
    }
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return result.QuadPart / _timer_highres_ticks_per_us;
}

double timer_us_precision() { return 1.0f / (double)_timer_highres_ticks_per_us; }

#define WIN_LOCK(lock) ((CRITICAL_SECTION*)(lock))
lock_t* lock_init() {
    CRITICAL_SECTION* lock_obj = (CRITICAL_SECTION*)malloc(sizeof(CRITICAL_SECTION));
    InitializeCriticalSection(lock_obj);
    return (lock_t*)lock_obj;
}
void lock(lock_t* lock_obj) { EnterCriticalSection(WIN_LOCK(lock_obj)); }
bool lock_try(lock_t* lock_obj) { return TryEnterCriticalSection(WIN_LOCK(lock_obj)); }
void lock_unlock(lock_t* lock_obj) { LeaveCriticalSection(WIN_LOCK(lock_obj)); }
void lock_destroy(lock_t* lock_obj) {
    DeleteCriticalSection(WIN_LOCK(lock_obj));
    free(lock_obj);
}

dlib_t* library_load(char* path) { return (dlib_t*)LoadLibrary(path); }
void* library_get(dlib_t* library, char* func_name) {
    return (void*)GetProcAddress((HMODULE)library, func_name);
}
void library_unload(dlib_t* library) { FreeLibrary((HMODULE)library); }

int create_directory(char* path) { return CreateDirectory(path, NULL); }
