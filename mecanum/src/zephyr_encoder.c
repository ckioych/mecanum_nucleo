#include "zephyr_encoder.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(encoder, LOG_LEVEL_INF);


static void encoder_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    // Получаем указатель на структуру энкодера
    zephyr_encoder_t *enc = CONTAINER_OF(cb, zephyr_encoder_t, callback);
    
    // Увеличиваем или уменьшаем счетчик в зависимости от направления
    if (enc->direction > 0) {
        enc->counter++;
    } else if (enc->direction < 0) {
        enc->counter--;
    } else {
        // Если направление не определено, считаем в плюс
        enc->counter++;
    }
}

bool zephyr_encoder_init(zephyr_encoder_t *enc, 
                         const struct device *gpio_dev, 
                         gpio_pin_t pin)
{
    enc->gpio_dev = gpio_dev;
    enc->pin = pin;
    enc->counter = 0;
    enc->last_count = 0;
    enc->last_time = k_uptime_get();
    enc->direction = 1;  // по умолчанию вперед
    
    if (!device_is_ready(gpio_dev)) {
        LOG_ERR("GPIO device not ready for encoder");
        return false;
    }
    
    // Настройка пина как вход с подтяжкой
    gpio_pin_configure(gpio_dev, pin, GPIO_INPUT | GPIO_PULL_UP);
    
    // Настройка прерывания по нарастающему фронту
    gpio_pin_interrupt_configure(gpio_dev, pin, GPIO_INT_EDGE_RISING);
    
    // Инициализация callback
    gpio_init_callback(&enc->callback, encoder_isr, BIT(pin));
    gpio_add_callback(gpio_dev, &enc->callback);
    
    LOG_INF("Encoder initialized on pin %d", pin);
    return true;
}

int32_t zephyr_encoder_get_count(zephyr_encoder_t *enc)
{
    return enc->counter;
}

int32_t zephyr_encoder_get_speed(zephyr_encoder_t *enc)
{
    int64_t now = k_uptime_get();
    int32_t current = enc->counter;
    int32_t delta = current - enc->last_count;
    int64_t dt = now - enc->last_time;
    
    if (dt > 0) {
        int32_t speed = delta * 1000 / dt;
        enc->last_count = current;
        enc->last_time = now;
        return speed;
    }
    
    return 0;
}

void zephyr_encoder_reset(zephyr_encoder_t *enc)
{
    enc->counter = 0;
    enc->last_count = 0;
    enc->last_time = k_uptime_get();
}

void zephyr_encoder_set_direction(zephyr_encoder_t *enc, int8_t dir)
{
    enc->direction = dir;
}