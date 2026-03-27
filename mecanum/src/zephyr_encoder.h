#ifndef ZEPHYR_ENCODER_H
#define ZEPHYR_ENCODER_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

// структура энкодера
typedef struct {
    const struct device *gpio_dev;
    gpio_pin_t pin;
    struct gpio_callback callback;
    
    volatile int32_t counter;
    volatile int32_t last_count;
    int64_t last_time;
    
    int8_t direction;
} zephyr_encoder_t;

// Инициализация энкодера
bool zephyr_encoder_init(zephyr_encoder_t *enc, 
                         const struct device *gpio_dev, 
                         gpio_pin_t pin);

// получение текущего значение счетчика
int32_t zephyr_encoder_get_count(zephyr_encoder_t *enc);

// получение скорости
int32_t zephyr_encoder_get_speed(zephyr_encoder_t *enc);

// сброс счетчика
void zephyr_encoder_reset(zephyr_encoder_t *enc);

// обновление направления
void zephyr_encoder_set_direction(zephyr_encoder_t *enc, int8_t dir);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ENCODER_H */
