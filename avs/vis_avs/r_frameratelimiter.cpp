#include "c_frameratelimiter.h"

#include <chrono>
#include <thread>

C_FramerateLimiter::C_FramerateLimiter()
    : enabled(true),
      framerate_limit(30),
      time_diff(1000 / 30),
      last_time(this->now()) {}

C_FramerateLimiter::~C_FramerateLimiter() {}

inline int64_t C_FramerateLimiter::now() {
    /* Don't ask me... */
    return std::chrono::time_point_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now())
        .time_since_epoch()
        .count();
}

inline void C_FramerateLimiter::sleep(int32_t duration_ms) {
    std::this_thread::sleep_for(
        std::chrono::duration<int32_t, std::milli>(duration_ms));
}

int C_FramerateLimiter::render(char[2][2][576], int, int*, int*, int, int) {
    if (this->enabled) {
        auto cur_time = this->now();
        if (cur_time - this->last_time <= this->time_diff) {
            int32_t sleep_time_ms = this->last_time - this->now() + this->time_diff;
            this->sleep(sleep_time_ms);
        }
    }
    this->last_time = this->now();
    return 0;
}

void C_FramerateLimiter::update_time_diff() {
    this->time_diff = 1000 / this->framerate_limit;
}

char* C_FramerateLimiter::get_desc(void) { return MOD_NAME; }

void C_FramerateLimiter::load_config(unsigned char* data, int len) {
    int32_t* int_data = (int32_t*)data;
    if (len >= 4) {
        this->enabled = int_data[0] != 0;
    } else {
        this->enabled = true;
    }
    if (len >= 8) {
        this->framerate_limit = int_data[1];
    } else {
        this->framerate_limit = this->default_framerate_limit;
    }
    this->update_time_diff();
}

int C_FramerateLimiter::save_config(unsigned char* data) {
    int32_t* int_data = (int32_t*)data;
    int_data[0] = this->enabled ? 1 : 0;
    int_data[1] = this->framerate_limit;
    return 8;
}

C_RBASE* R_FramerateLimiter(char* desc) {
    if (desc) {
        strcpy(desc, MOD_NAME);
        return NULL;
    }
    return (C_RBASE*)new C_FramerateLimiter();
}
