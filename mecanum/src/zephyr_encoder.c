#include "zephyr_encoder.h"
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <string.h>

LOG_MODULE_REGISTER(zephyr_encoder, LOG_LEVEL_INF);

// Для аппаратных энкодеров (Quadrature decoder)
#ifdef CONFIG_SENSOR
#include <zephyr/drivers/sensor.h>
#endif

// Обработчик прерывания для пина A (программный режим)
void Encoder_irq_handler_a(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    struct Encoder *enc = CONTAINER_OF(cb, struct Encoder, gpio_cb_a);
    
    uint8_t state_a = gpio_pin_get_dt(&enc->pin_a);
    uint8_t state_b = gpio_pin_get_dt(&enc->pin_b);
    
    // Определение направления по состоянию пинов
    if (state_a) {
        // По переднему фронту A
        if (state_b) {
            enc->counter--;  // B высокий - вращение назад
        } else {
            enc->counter++;  // B низкий - вращение вперед
        }
    }
}

// Обработчик прерывания для пина B (программный режим)
void Encoder_irq_handler_b(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    struct Encoder *enc = CONTAINER_OF(cb, struct Encoder, gpio_cb_b);
    
    uint8_t state_a = gpio_pin_get_dt(&enc->pin_a);
    uint8_t state_b = gpio_pin_get_dt(&enc->pin_b);
    
    // Определение направления по состоянию пинов
    if (state_b) {
        // По переднему фронту B
        if (state_a) {
            enc->counter++;  // A высокий - вращение вперед
        } else {
            enc->counter--;  // A низкий - вращение назад
        }
    }
}

// Инициализация программного энкодера
int Encoder_init_software(struct Encoder *enc, uint32_t pin_a, uint32_t pin_b) {
    memset(enc, 0, sizeof(struct Encoder));
    
    enc->use_hardware = false;
    enc->counter = 0;
    enc->last_counter = 0;
    enc->speed = 0;
    
    // Настройка пина A
    enc->pin_a.port = DEVICE_DT_GET(DT_NODELABEL(gpioa)); // Пример, нужно настроить через DT
    enc->pin_a.pin = pin_a;
    enc->pin_a.dt_flags = GPIO_INPUT | GPIO_INT_ENABLE;
    
    if (!device_is_ready(enc->pin_a.port)) {
        LOG_ERR("GPIO port for pin A not ready");
        return -ENODEV;
    }
    
    // Настройка пина B
    enc->pin_b.port = DEVICE_DT_GET(DT_NODELABEL(gpioa));
    enc->pin_b.pin = pin_b;
    enc->pin_b.dt_flags = GPIO_INPUT | GPIO_INT_ENABLE;
    
    if (!device_is_ready(enc->pin_b.port)) {
        LOG_ERR("GPIO port for pin B not ready");
        return -ENODEV;
    }
    
    // Конфигурация пинов
    gpio_pin_configure_dt(&enc->pin_a, GPIO_INPUT);
    gpio_pin_configure_dt(&enc->pin_b, GPIO_INPUT);
    
    // Настройка прерываний на оба фронта
    gpio_pin_interrupt_configure_dt(&enc->pin_a, GPIO_INT_EDGE_BOTH);
    gpio_pin_interrupt_configure_dt(&enc->pin_b, GPIO_INT_EDGE_BOTH);
    
    // Регистрация callback'ов
    gpio_init_callback(&enc->gpio_cb_a, Encoder_irq_handler_a, BIT(pin_a));
    gpio_init_callback(&enc->gpio_cb_b, Encoder_irq_handler_b, BIT(pin_b));
    
    gpio_add_callback(enc->pin_a.port, &enc->gpio_cb_a);
    gpio_add_callback(enc->pin_b.port, &enc->gpio_cb_b);
    
    LOG_INF("Software encoder initialized on pins %d, %d", pin_a, pin_b);
    return 0;
}

// Инициализация аппаратного энкодера
int Encoder_init_hardware(struct Encoder *enc, const char *sensor_dev_name) {
    memset(enc, 0, sizeof(struct Encoder));
    
    enc->use_hardware = true;
    enc->dev_name = sensor_dev_name;
    enc->counter = 0;
    enc->last_counter = 0;
    enc->speed = 0;
    
    // Получение устройства
    enc->sensor_dev = device_get_binding(sensor_dev_name);
    if (!enc->sensor_dev) {
        LOG_ERR("Encoder device %s not found", sensor_dev_name);
        return -ENODEV;
    }
    
    LOG_INF("Hardware encoder initialized: %s", sensor_dev_name);
    return 0;
}

// Чтение аппаратного энкодера
int32_t Encoder_read_hardware(struct Encoder *enc) {
    if (!enc->use_hardware || !enc->sensor_dev) {
        return enc->counter;
    }
    
    struct sensor_value val;
    if (sensor_sample_fetch(enc->sensor_dev) < 0) {
        LOG_ERR("Failed to fetch encoder sample");
        return enc->counter;
    }
    
    if (sensor_channel_get(enc->sensor_dev, SENSOR_CHAN_ROTATION, &val) < 0) {
        LOG_ERR("Failed to get encoder rotation");
        return enc->counter;
    }
    
    enc->counter = val.val1;
    return enc->counter;
}

// Получение текущей позиции
int32_t Encoder_get_position(struct Encoder *enc) {
    if (enc->use_hardware) {
        return Encoder_read_hardware(enc);
    } else {
        // Для программного режима просто возвращаем счетчик
        // (обновляется в прерываниях)
        return enc->counter;
    }
}

// Обновление скорости (вызывать периодически)
void Encoder_update_speed(struct Encoder *enc) {
    uint32_t now = k_uptime_get_32();
    int32_t current_pos = Encoder_get_position(enc);
    
    if (enc->last_speed_update != 0) {
        uint32_t dt = now - enc->last_speed_update;
        if (dt > 0) {
            enc->speed = (current_pos - enc->last_counter) * 1000 / dt;
        }
    }
    
    enc->last_counter = current_pos;
    enc->last_speed_update = now;
}

// Получение скорости
int32_t Encoder_get_speed(struct Encoder *enc) {
    return enc->speed;
}

// Сброс счетчика
void Encoder_reset(struct Encoder *enc) {
    enc->counter = 0;
    enc->last_counter = 0;
    enc->speed = 0;
    enc->last_speed_update = 0;
}