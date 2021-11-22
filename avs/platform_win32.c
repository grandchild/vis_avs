#include "platform.h"

#include <windows.h>
#include <commctrl.h>
#include <windowsx.h>

uint32_t millis_since_boot() {
    uint32_t millis = GetTickCount();
    // Create the same behavior as the Linux version, which returns 0 on error.
    return millis != 0 ? millis : 1;
}

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
