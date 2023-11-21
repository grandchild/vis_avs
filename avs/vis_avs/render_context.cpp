#include "render_context.h"

template <>
int16_t AudioChannels<int16_t>::avg(int16_t l, int16_t r) {
    return (int16_t)(((int32_t)l + (int32_t)r) / 2);
}

template <>
uint16_t AudioChannels<uint16_t>::avg(uint16_t l, uint16_t r) {
    return (uint16_t)(((uint32_t)l + (uint32_t)r) / 2);
}
