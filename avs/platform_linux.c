#include "platform.h"

#include "3rdparty/pevents.h"

#include <dlfcn.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <sys/mman.h>  // mprotect flags
#include <sys/stat.h>  // mkdir()

/* timers & time */

uint64_t timer_ms() {
    struct timespec ts;
    if (clock_gettime(CLOCK_BOOTTIME, &ts) == -1) {
        return 0;
    }
    return ts.tv_sec * 1000 + ts.tv_nsec / (1000 * 1000);
}

uint64_t timer_us() {
    struct timespec ts;
    if (clock_gettime(CLOCK_BOOTTIME, &ts) == -1) {
        return 0;
    }
    return ts.tv_sec * 1000 * 1000 + ts.tv_nsec / 1000;
}

double timer_us_precision() {
    struct timespec ts;
    if (clock_getres(CLOCK_BOOTTIME, &ts) == -1) {
        return 0.0;
    }
    return ts.tv_sec * 1000.0 * 1000.0 + (double)ts.tv_nsec / 1000.0;
}

const char* current_date_str() {
    static char date_str[11];
    time_t t = time(NULL);
    struct tm date = *localtime(&t);
    snprintf(date_str,
             11,
             "%04d-%02d-%02d",
             date.tm_year + 1900,
             date.tm_mon + 1,
             date.tm_mday);
    return date_str;
}

/* locking */

#define PTHREAD_LOCK(lock) ((pthread_mutex_t*)(lock))
lock_t* lock_init() {
    pthread_mutex_t* lock_obj = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    // Make the mutex recursive because Win32's CRITICAL_SECTIONs are, and we want locks
    // to behave the same everywhere.
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init((pthread_mutex_t*)lock_obj, &attr);
    return (lock_t*)lock_obj;
}
void lock_lock(lock_t* lock_obj) { pthread_mutex_lock(PTHREAD_LOCK(lock_obj)); }
bool lock_try(lock_t* lock_obj) {
    return 0 != pthread_mutex_trylock(PTHREAD_LOCK(lock_obj));
}
void lock_unlock(lock_t* lock_obj) { pthread_mutex_unlock(PTHREAD_LOCK(lock_obj)); }
void lock_destroy(lock_t* lock_obj) {
    pthread_mutex_destroy(PTHREAD_LOCK(lock_obj));
    free(lock_obj);
}

/* signals */
// TODO

/*
typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    bool is_broadcast;
} linux_signal_t;

static linux_signal_t* _signal_create(bool is_broadcast) {
    linux_signal_t* signal = (linux_signal_t*)malloc(sizeof(linux_signal_t));
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutex_init(&signal->mutex, &attr);
    pthread_cond_init(&signal->condition, NULL);
    signal->is_broadcast = is_broadcast;
    return signal;
}
signal_t* signal_create_single() { return _signal_create(false); }
signal_t* signal_create_broadcast() { return _signal_create(true); }
void signal_set(signal_t* signal) {
    linux_signal_t* lsignal = (linux_signal_t*)signal;
    pthread_mutex_lock(&lsignal->mutex);
    if (lsignal->is_broadcast) {
        pthread_cond_broadcast(&lsignal->condition);
    } else {
        pthread_cond_signal(&lsignal->condition);
    }
    pthread_mutex_unlock(&lsignal->mutex);
}
void signal_unset(signal_t* signal) {}
signal_t* signal_wait(signal_t* signal, int32_t wait_ms) {
    signal_t* result = NULL;
    linux_signal_t* lsignal = (linux_signal_t*)signal;
    pthread_mutex_lock(&lsignal->mutex);
    if (wait_ms == WAIT_INFINITE) {
        pthread_cond_wait(&lsignal->condition, &lsignal->mutex);
        result = signal;
    } else {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += wait_ms / 1000;
        ts.tv_nsec += (wait_ms % 1000) * (1000 * 1000);
        int ret = pthread_cond_timedwait(&lsignal->condition, &lsignal->mutex, &ts);
        if (ret != ETIMEDOUT) {
            result = signal;
        }
    }
    pthread_mutex_unlock(&lsignal->mutex);
    return result;
}
signal_t* signal_wait_any(signal_t** signals, uint32_t num_signals, int32_t wait_ms) {
    signal_t* result = NULL;
    linux_signal_t** lsignals = (linux_signal_t**)signals;
    struct timespec wait_timeout;
    clock_gettime(CLOCK_REALTIME, &wait_timeout);
    wait_timeout.tv_sec += wait_ms / 1000;
    wait_timeout.tv_nsec += (wait_ms % 1000) * (1000 * 1000);
    for (uint32_t i = 0; i < num_signals; i++) {
        struct timespec switch_timeout;
        clock_gettime(CLOCK_REALTIME, &switch_timeout);
        int32_t switch_divisor = 100;
        switch_timeout.tv_sec += wait_ms / 1000 / switch_divisor;
        switch_timeout.tv_nsec +=
            (wait_ms % (1000 * switch_divisor)) * (1000 * 1000 / switch_divisor);
        struct timespec* timeout = switch_timeout.tv_sec < wait_timeout.tv_sec
                                       ? &switch_timeout
                                       : &wait_timeout;
        pthread_mutex_lock(&lsignals[i]->mutex);
        int ret = pthread_cond_timedwait(
            &lsignals[i]->condition, &lsignals[i]->mutex, timeout);
        pthread_mutex_unlock(&lsignals[i]->mutex);
        if (ret == ETIMEDOUT) {
            result = NULL;
            if (timeout == &wait_timeout) {
                break;
            }
        }
        result = signals[i];
    }
    return result;
}
signal_t* signal_wait_all(signal_t** signals, uint32_t num_signals, int32_t wait_ms) {
    signal_t* result = NULL;
    linux_signal_t** lsignals = (linux_signal_t**)signals;
    if (wait_ms == WAIT_INFINITE) {
        for (uint32_t i = 0; i < num_signals; i++) {
            pthread_mutex_lock(&lsignals[i]->mutex);
            pthread_cond_wait(&lsignals[i]->condition, &lsignals[i]->mutex);
            pthread_mutex_unlock(&lsignals[i]->mutex);
        }
        result = signals[0];
    } else {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += wait_ms / 1000;
        ts.tv_nsec += (wait_ms % 1000) * (1000 * 1000);
        for (uint32_t i = 0; i < num_signals; i++) {
            pthread_mutex_lock(&lsignals[i]->mutex);
            int ret = pthread_cond_timedwait(
                &lsignals[i]->condition, &lsignals[i]->mutex, &ts);
            pthread_mutex_unlock(&lsignals[i]->mutex);
            if (ret == ETIMEDOUT) {
                result = NULL;
                break;
            }
            result = signals[i];
        }
    }
    return result;
}
void signal_destroy(signal_t* signal) {}
//*/

signal_t* signal_create_single() {
    neosmart_event_t event = NspeCreateEvent(false, false);
    return event;
}
signal_t* signal_create_broadcast() {
    neosmart_event_t event = NspeCreateEvent(true, false);
    return event;
}
void signal_set(signal_t* signal) { NspeSetEvent(signal); }
void signal_unset(signal_t* signal) { NspeResetEvent(signal); }
signal_t* signal_wait(signal_t* signal, int32_t wait_ms) {
    if (wait_ms == WAIT_INFINITE) {
        NspeWaitForEvent(signal, NSPE_WAIT_INFINITE);
        return signal;
    }
    int result = NspeWaitForEvent(signal, wait_ms);
    if (result == WAIT_TIMEOUT) {
        return NULL;
    }
    return signal;
}
signal_t* signal_wait_any(signal_t** signals, uint32_t num_signals, int32_t wait_ms) {
    neosmart_event_t* linux_signals = (neosmart_event_t*)signals;
    if (wait_ms == WAIT_INFINITE) {
        return signals[NspeWaitForMultipleEvents(
            linux_signals, num_signals, false, NSPE_WAIT_INFINITE)];
    }
    int result = NspeWaitForMultipleEvents(linux_signals, num_signals, false, wait_ms);
    if (result == WAIT_TIMEOUT) {
        return NULL;
    }
    return signals[result];
}
signal_t* signal_wait_all(signal_t** signals, uint32_t num_signals, int32_t wait_ms) {
    neosmart_event_t* linux_signals = (neosmart_event_t*)signals;
    if (wait_ms == WAIT_INFINITE) {
        return signals[NspeWaitForMultipleEvents(
            linux_signals, num_signals, true, NSPE_WAIT_INFINITE)];
    }
    int result = NspeWaitForMultipleEvents(linux_signals, num_signals, true, wait_ms);
    if (result == WAIT_TIMEOUT) {
        return NULL;
    }
    return signals[result];
}
void signal_destroy(signal_t* signal) { NspeDestroyEvent(signal); }

/* threading */

// TODO
//*
thread_t* thread_create(uint32_t (*func)(void* data), void* data) {
    pthread_t* thread = malloc(sizeof(pthread_t));
    pthread_create(thread, NULL, (void* (*)(void*))func, data);
    return (thread_t*)thread;
}
thread_t* thread_current() { return (thread_t*)pthread_self(); }
bool thread_join(thread_t* thread, int32_t wait_ms) {
    pthread_t* pthread = (pthread_t*)thread;
    // if (wait_ms == WAIT_INFINITE) {
    pthread_join(*pthread, NULL);
    return true;
    // }
    // struct timespec ts;
    // clock_gettime(CLOCK_REALTIME, &ts);
    // ts.tv_sec += wait_ms / 1000;
    // ts.tv_nsec += (wait_ms % 1000) * (1000 * 1000);
    // return pthread_timedjoin_np(*pthread, NULL, &ts) == 0;
}
bool thread_join_all(thread_t** threads, uint32_t num_threads, int32_t wait_ms) {
    bool result = true;
    for (uint32_t i = 0; i < num_threads; i++) {
        result &= thread_join(threads[i], wait_ms);
    }
    return result;
}
bool thread_decrease_priority(thread_t* thread) {
    return pthread_setschedprio(*(pthread_t*)thread, 0) == 0;
}
void thread_destroy(thread_t* thread) { free(thread); }
//*/

/* dynamic library loading */

dlib_t* library_load(const char* path) {
    return (dlib_t*)dlopen(path, RTLD_LAZY | RTLD_GLOBAL);
}
func_t library_get(dlib_t* library, const char* func_name) {
    dlerror();
    if (library == NULL) {
        return NULL;
    }
    void* function = dlsym(library, func_name);
    if (dlerror() != NULL) {
        return NULL;
    }
    return function;
}
void library_unload(dlib_t* library) {
    if (library != NULL) {
        dlclose(library);
    }
}
const char* library_error() { return dlerror(); }

/* mkdir */

int create_directory(const char* path) { return mkdir(path, 0777) != 0; }

/* memory read-write-exec */

void mem_mark_rwx(void* block, size_t length) {
    static int pagesize = 0;
    if (!pagesize) {
        pagesize = sysconf(_SC_PAGESIZE);
        if (!pagesize) {
            pagesize = 4096;
        }
    }
    void* block_aligned = (void*)(((unsigned int)block) & ~(pagesize - 1));
    size_t length_aligned =
        (((unsigned int)block + length + pagesize - 1) & ~(pagesize - 1))
        - (unsigned int)block_aligned;
    mprotect((void*)block_aligned, length_aligned, PROT_WRITE | PROT_READ | PROT_EXEC);
}
