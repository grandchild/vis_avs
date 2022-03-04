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

/* locking */

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

/* signals */

static signal_t* _signal_create(bool manual) {
    return CreateEvent(NULL, manual, false, NULL);
}

signal_t* signal_create_single() { return _signal_create(/*manual*/ false); }

signal_t* signal_create_broadcast() { return _signal_create(/*manual*/ true); }

void signal_set(signal_t* signal) { SetEvent(signal); }

signal_t* signal_wait(signal_t* signal, int32_t wait_ms) {
    uint32_t wait = wait_ms == WAIT_INFINITE ? INFINITE : (uint32_t)wait_ms;
    uint32_t wait_result = WaitForSingleObject(signal, wait);
    return wait_result == WAIT_OBJECT_0 ? signal : NULL;
}

static signal_t* _signal_wait_multiple(signal_t** signals,
                                       uint32_t num_signals,
                                       int32_t wait_ms,
                                       bool wait_for_all) {
    uint32_t wait = wait_ms == WAIT_INFINITE ? INFINITE : (uint32_t)wait_ms;
    uint32_t wait_result =
        WaitForMultipleObjects(num_signals, signals, wait_for_all, wait);
    if (wait_result < WAIT_ABANDONED_0 && wait_result < num_signals) {
        return signals[wait_result - WAIT_OBJECT_0];
    } else {
        return NULL;
    }
}

signal_t* signal_wait_any(signal_t** signals, uint32_t num_signals, int32_t wait_ms) {
    return _signal_wait_multiple(signals, num_signals, wait_ms, /*wait_for_all*/ false);
}

signal_t* signal_wait_all(signal_t** signals, uint32_t num_signals, int32_t wait_ms) {
    return _signal_wait_multiple(signals, num_signals, wait_ms, /*wait_for_all*/ true);
}

void signal_destroy(signal_t* signal) { CloseHandle(signal); }

/* threading */

typedef long unsigned int(__stdcall* win32_threadproc_type)(void*);
thread_t* thread_create(uint32_t (*func)(void* data), void* data) {
    return CreateThread(NULL, 0, (win32_threadproc_type)func, data, 0, NULL);
}

bool thread_join(thread_t* thread, int32_t wait_ms) {
    uint32_t wait = wait_ms == WAIT_INFINITE ? INFINITE : (uint32_t)wait_ms;
    return WaitForSingleObject(thread, wait) == WAIT_OBJECT_0;
}

bool thread_join_all(thread_t** threads, uint32_t num_threads, int32_t wait_ms) {
    uint32_t wait = wait_ms == WAIT_INFINITE ? INFINITE : (uint32_t)wait_ms;
    uint32_t wait_result = WaitForMultipleObjects(num_threads, threads, true, wait);
    return wait_result < WAIT_ABANDONED_0 && wait_result < num_threads;
}

void thread_destroy(thread_t* thread) { CloseHandle(thread); }

/* dynamic library loading */

dlib_t* library_load(char* path) { return (dlib_t*)LoadLibrary(path); }

void* library_get(dlib_t* library, char* func_name) {
    return (void*)GetProcAddress((HMODULE)library, func_name);
}

void library_unload(dlib_t* library) { FreeLibrary((HMODULE)library); }

/* mkdir */

int create_directory(char* path) { return CreateDirectory(path, NULL); }
