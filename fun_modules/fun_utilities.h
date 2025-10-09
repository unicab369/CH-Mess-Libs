#include "ch32fun.h"
#include <stdint.h>
#include <stdio.h>


typedef struct {
    u32 count;
    u32 limit_counter;
    u32 time_max;
    u16 time_min;
} Cycle_Info_t;

void UTIL_cycleInfo_clear(Cycle_Info_t *info) {
    info->count = 0;
    info->limit_counter = 0;
    info->time_max = 0;
    info->time_min = 0xFFFF;
}

void UTIL_cycleInfo_flush(Cycle_Info_t *info) {
    printf("\nCycle count: %d, max/min: %d/%d us, lim_counter: %d\n",
            info->count, info->time_max, info->time_min, info->limit_counter);
    UTIL_cycleInfo_clear(info);
}

void UTIL_cycleInfo_update(Cycle_Info_t *info, u32 elapsed) {
    info->count++;
    if (elapsed > info->time_max) info->time_max = elapsed;
    if (elapsed < info->time_min) info->time_min = elapsed;
}

void UTIL_cycleInfo_updateWithLimit(Cycle_Info_t *info, u32 elapsed, u32 limit) {
    UTIL_cycleInfo_update(info, elapsed);
    if (elapsed < limit) info->limit_counter++;
}




typedef struct {
	u32 max;
	u32 min;
} MinMax_Info32_t;

void UTIL_minMax_clear(MinMax_Info32_t *info) {
    info->max = 0;
    info->min = 0xFFFFFFFF;
}

void UTIL_minMax_flush(MinMax_Info32_t *info) {
    printf("MinMax Max: %d, Min: %d\n", info->max, info->min);
    UTIL_minMax_clear(info);
}

void UTIL_minMax_updateMin(MinMax_Info32_t *info, u32 value) {
    if (value < info->min) info->min = value;
}

void UTIL_minMax_updateMax(MinMax_Info32_t *info, u32 value) {
    if (value > info->max) info->max = value;
}

const char* UTIL_minMax_getStr(MinMax_Info32_t *info, u32 value) {
    return (value == info->max) ? "(max)" :
            (value == info->min) ? "(min)" : "";
}



typedef struct {
    u32 *buf;
    u16 buf_idx;
    u16 buf_len;
    u32 threshold_limit;
} Threshold_Buffer_t;

void fun_thresholdBuffer_clear(Threshold_Buffer_t *info) {
    memset(info->buf, 0, sizeof(u32) * info->buf_len);
    info->buf_idx = 0;
}

void fun_thresholdBuffer_flush(Threshold_Buffer_t *info) {
    printf("\nThreshold: %d, count: %d\n", info->threshold_limit, info->buf_idx);

    for (int i = 0; i < info->buf_idx; i++) {
        printf("%d ", info->buf[i]);
    }
    printf("\n");

    fun_thresholdBuffer_clear(info);
}

u8 fun_thresholdBuffer_addUpperValue(Threshold_Buffer_t *info, u32 value) {
    if (info->buf_idx + 1 >= info->buf_len) return 0;
    if (value < info->threshold_limit) return 0;

    info->buf[info->buf_idx] = value;
    info->buf_idx++;
    return 1;
}


typedef struct {
    u16 *buf;
    u16 buf_idx;
    u16 buf_len;
} Debug_Buffer_t;

void UTIL_DebugBuffer_clear(Debug_Buffer_t *model) {
    memset(model->buf, 0, sizeof(u16) * model->buf_len);
    model->buf_idx = 0;
}

void UTIL_DebugBuffer_flush(Debug_Buffer_t *model) {
    if (model->buf_idx < 1) return;
    printf("\nDEBUG BUFFER\n");

    for (int i = 0; i < model->buf_idx; i++) {
        u8 bit = model->buf[i] > 1000;
        printf("[%3d] %d \t%d\n", i, model->buf[i], bit);
        if (i%8 == 7) printf("\n");
    }
    printf("\n");

    UTIL_DebugBuffer_clear(model);
}

void UTIL_DebugBuffer_addValue(Debug_Buffer_t *model, u16 value) {
    if (model->buf_idx + 1 >= model->buf_len) return;
    model->buf[model->buf_idx] = value;
    model->buf_idx++;
}