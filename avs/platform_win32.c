#include "platform.h"

#include <windows.h>
#include <commctrl.h>
#include <windowsx.h>

/* timers */

static int64_t timer_highres_ticks_per_us = 0;

static void timer_us_init() {
    LARGE_INTEGER ticks_per_second = {.QuadPart = 0};
    if (!QueryPerformanceFrequency(&ticks_per_second)) {
        timer_highres_ticks_per_us = 1;
    } else {
        timer_highres_ticks_per_us = ticks_per_second.QuadPart / (1000 * 1000);
    }
}

uint64_t timer_ms() { return GetTickCount64(); }

uint64_t timer_us() {
    if (timer_highres_ticks_per_us == 0) {
        timer_us_init();
    }
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return result.QuadPart / timer_highres_ticks_per_us;
}

double timer_us_precision() {
    if (timer_highres_ticks_per_us == 0) {
        timer_us_init();
    }
    return 1.0f / (double)timer_highres_ticks_per_us;
}

/* locking */

#define WIN_LOCK(lock) ((CRITICAL_SECTION*)(lock))

lock_t* lock_init() {
    CRITICAL_SECTION* lock_obj = (CRITICAL_SECTION*)malloc(sizeof(CRITICAL_SECTION));
    InitializeCriticalSection(lock_obj);
    return (lock_t*)lock_obj;
}

void lock_lock(lock_t* lock_obj) { EnterCriticalSection(WIN_LOCK(lock_obj)); }

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

void signal_unset(signal_t* signal) { ResetEvent(signal); }

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

thread_t* thread_current() { return GetCurrentThread(); }

bool thread_join(thread_t* thread, int32_t wait_ms) {
    uint32_t wait = wait_ms == WAIT_INFINITE ? INFINITE : (uint32_t)wait_ms;
    return WaitForSingleObject(thread, wait) == WAIT_OBJECT_0;
}

bool thread_join_all(thread_t** threads, uint32_t num_threads, int32_t wait_ms) {
    uint32_t wait = wait_ms == WAIT_INFINITE ? INFINITE : (uint32_t)wait_ms;
    uint32_t wait_result = WaitForMultipleObjects(num_threads, threads, true, wait);
    return wait_result < WAIT_ABANDONED_0 && wait_result < num_threads;
}

bool thread_decrease_priority(thread_t* thread) {
    if (thread == NULL) {
        return false;
    }
    int cur_prio = GetThreadPriority(thread);
    if (cur_prio == THREAD_PRIORITY_ERROR_RETURN || cur_prio == THREAD_PRIORITY_IDLE) {
        return false;
    }
    int lower_prio = cur_prio;
    if (cur_prio == THREAD_PRIORITY_TIME_CRITICAL) {
        lower_prio = THREAD_PRIORITY_HIGHEST;
    } else if (cur_prio == THREAD_PRIORITY_LOWEST) {
        lower_prio = THREAD_PRIORITY_IDLE;
    } else {
        lower_prio -= 1;
    }
    return (bool)SetThreadPriority(thread, lower_prio);
}

void thread_destroy(thread_t* thread) { CloseHandle(thread); }

/* dynamic library loading */

// TODO [security]: Disallow loading DLLs from CWD with LOAD_LIBRARY_SAFE_CURRENT_DIRS
//                  or similar.
dlib_t* library_load(const char* path) { return (dlib_t*)LoadLibrary(path); }

func_t library_get(dlib_t* library, const char* func_name) {
    return (void*)GetProcAddress((HMODULE)library, func_name);
}

void library_unload(dlib_t* library) {
    if (library != NULL) {
        FreeLibrary((HMODULE)library);
    }
}

/* mkdir */

int create_directory(const char* path) { return CreateDirectory(path, NULL); }

/* memory read-write-exec */

void mem_mark_rwx(void* block, size_t length) {
    unsigned int alignment_mask = ~4095;
    void* block_aligned = (void*)((unsigned int)block & alignment_mask);
    size_t length_aligned = (((unsigned int)block + length + 4095) & alignment_mask)
                            - (unsigned int)block_aligned;
    VirtualProtect(block_aligned, length_aligned, PAGE_EXECUTE_READWRITE, NULL);
}
