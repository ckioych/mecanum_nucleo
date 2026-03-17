#ifndef ZEPHYR_ENCODER_H
#define ZEPHYR_ENCODER_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/counter.h>

#ifdef __cplusplus
extern "C" {
#endif

struct Encoder {
    const struct device *sensor_dev;
    
    struct gpio_dt_spec pin_a;
    struct gpio_dt_spec pin_b;
    struct gpio_callback gpio_cb_a;
    struct gpio_callback gpio_cb_b;
    
    volatile int32_t counter;
    volatile int32_t last_counter;
    int32_t speed;
    uint32_t last_speed_update;
    
    uint8_t last_state_a;
    uint8_t last_state_b;
    int8_t direction;
    
    bool use_hardware;
    
    const char *dev_name;
};

int Encoder_init_software(struct Encoder *enc, uint32_t pin_a, uint32_t pin_b);

int Encoder_init_hardware(struct Encoder *enc, const char *sensor_dev_name);

int32_t Encoder_get_position(struct Encoder *enc);

int32_t Encoder_get_speed(struct Encoder *enc);

void Encoder_reset(struct Encoder *enc);

void Encoder_update_speed(struct Encoder *enc);

void Encoder_irq_handler_a(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
void Encoder_irq_handler_b(const struct device *dev, struct gpio_callback *cb, uint32_t pins);

int32_t Encoder_read_hardware(struct Encoder *enc);

#ifdef __cplusplus
}
#endif

#endif // ZEPHYR_ENCODER_H