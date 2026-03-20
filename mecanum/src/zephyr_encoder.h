#ifndef ZEPHYR_ENCODER_H
#define ZEPHYR_ENCODER_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

typedef struct {
    const struct device *dev;    // устройство qdec
    int32_t last_count;          // последнее знач. для расчета скорости
    int64_t last_time;           // время послед. чтения
    int32_t offset;              // смещение для ручного сброса
} zephyr_encoder_t;

bool zephyr_encoder_init(zephyr_encoder_t *enc, const struct device *qdec_dev);

int32_t zephyr_encoder_get_count(zephyr_encoder_t *enc);
int32_t zephyr_encoder_get_speed(zephyr_encoder_t *enc);
void zephyr_encoder_reset(zephyr_encoder_t *enc);

#endif // ZEPHYR_ENCODER_H