#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include "zephyr_motor.h"
#include "zephyr_encoder.h"
#include "zephyr_accelmotor.h"
#include "zephyr_ps2.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

// конфигурация
#define MAX_SPEED           70      // макс. скорость в тиках/сек
#define MIN_DUTY            20      // минимальный ШИМ для трогания с места
#define STEP_SIZE           50      // шаг перемещения по кнопкам
#define ACCEL               7       // ускорение
#define MAX_FOLLOW_SPEED    500     // макс. скорость слежения
#define DT_MS               20      // период дискретизации
#define MOTOR_AUTOTEST_NO_PS2 1     // 1: автотест моторов без геймпада PS2
#define AUTOTEST_SPEED      30      // сниженный duty для щадящего теста силовой части
#define AUTOTEST_STEP_MS    2000

// ПИД коэф-ты
#define PID_P   2.2f
#define PID_I   0.4f
#define PID_D   0.01f

// моторы
static zephyr_motor_t motor_fl, motor_fr, motor_bl, motor_br;

#if !MOTOR_AUTOTEST_NO_PS2
// энкодеры
static zephyr_encoder_t enc_fl, enc_fr, enc_bl, enc_br;

// доп. функции моторов
static zephyr_accelmotor_t accel_fl, accel_fr, accel_bl, accel_br;
#endif

// PS2 контроллер
#if !MOTOR_AUTOTEST_NO_PS2
static ps2_t ps2;

// режим
static bool position_mode = false;  // false - speed mode, true - position mode
static int32_t pos_fl = 0, pos_fr = 0, pos_bl = 0, pos_br = 0;

static void update_target_positions(void) {
    zephyr_accelmotor_set_target_pos(&accel_fl, pos_fl);
    zephyr_accelmotor_set_target_pos(&accel_fr, pos_fr);
    zephyr_accelmotor_set_target_pos(&accel_bl, pos_bl);
    zephyr_accelmotor_set_target_pos(&accel_br, pos_br);
}

// переключение режимов
static void change_mode(bool mode) {
    position_mode = mode;

    if (mode) {
        // переключение всех моторов в режим position
        zephyr_accelmotor_set_run_mode(&accel_fl, ACCEL_POS);
        zephyr_accelmotor_set_run_mode(&accel_fr, ACCEL_POS);
        zephyr_accelmotor_set_run_mode(&accel_bl, ACCEL_POS);
        zephyr_accelmotor_set_run_mode(&accel_br, ACCEL_POS);

        // Обновляем текущие позиции
        pos_fl = zephyr_accelmotor_get_current_pos(&accel_fl);
        pos_fr = zephyr_accelmotor_get_current_pos(&accel_fr);
        pos_bl = zephyr_accelmotor_get_current_pos(&accel_bl);
        pos_br = zephyr_accelmotor_get_current_pos(&accel_br);

        update_target_positions();

        LOG_INF("Switched to POSITION mode");
    } else {
        // переключение всех моторов в режим speed
        zephyr_accelmotor_set_run_mode(&accel_fl, PID_SPEED);
        zephyr_accelmotor_set_run_mode(&accel_fr, PID_SPEED);
        zephyr_accelmotor_set_run_mode(&accel_bl, PID_SPEED);
        zephyr_accelmotor_set_run_mode(&accel_br, PID_SPEED);

        LOG_INF("Switched to SPEED mode");
    }
}
#endif

static K_MUTEX_DEFINE(control_lock);

// управление моторами
void motor_control_thread(void *, void *, void *) {
    LOG_INF("Motor control thread started");

    while (1) {
#if MOTOR_AUTOTEST_NO_PS2
        k_sleep(K_MSEC(100));
        continue;
#else
        static uint32_t last_print = 0;
        k_mutex_lock(&control_lock, K_FOREVER);

        // чтение энкодеров
        int32_t enc_fl_val = zephyr_encoder_get_count(&enc_fl);
        int32_t enc_fr_val = zephyr_encoder_get_count(&enc_fr);
        int32_t enc_bl_val = zephyr_encoder_get_count(&enc_bl);
        int32_t enc_br_val = zephyr_encoder_get_count(&enc_br);

        // Обновление моторов
        bool moving1 = zephyr_accelmotor_tick(&accel_fl, enc_fl_val);
        bool moving2 = zephyr_accelmotor_tick(&accel_fr, enc_fr_val);
        bool moving3 = zephyr_accelmotor_tick(&accel_bl, enc_bl_val);
        bool moving4 = zephyr_accelmotor_tick(&accel_br, enc_br_val);

        if (position_mode && !moving1 && !moving2 && !moving3 && !moving4) {
            LOG_INF("All motors stopped, switching to speed mode");
            change_mode(false);
        }

        // вывод отладочной информации
        uint32_t now = k_uptime_get_32();
        if (now - last_print > 1000) {
            last_print = now;

            if (position_mode) {
                LOG_INF("Pos: FL=%d FR=%d BL=%d BR=%d | Target: FL=%d FR=%d",
                        zephyr_accelmotor_get_current_pos(&accel_fl),
                        zephyr_accelmotor_get_current_pos(&accel_fr),
                        zephyr_accelmotor_get_current_pos(&accel_bl),
                        zephyr_accelmotor_get_current_pos(&accel_br),
                        zephyr_accelmotor_get_target_pos(&accel_fl),
                        zephyr_accelmotor_get_target_pos(&accel_fr));
            } else {
                LOG_INF("Speed: FL=%d FR=%d BL=%d BR=%d | Duty: FL=%.1f FR=%.1f",
                        zephyr_accelmotor_get_current_speed(&accel_fl),
                        zephyr_accelmotor_get_current_speed(&accel_fr),
                        zephyr_accelmotor_get_current_speed(&accel_bl),
                        zephyr_accelmotor_get_current_speed(&accel_br),
                        zephyr_accelmotor_get_duty(&accel_fl),
                        zephyr_accelmotor_get_duty(&accel_fr));
            }
        }

        k_mutex_unlock(&control_lock);
        k_sleep(K_MSEC(DT_MS));
#endif
    }
}

// PS2
void ps2_thread(void *, void *, void *) {
    LOG_INF("PS2 thread started");

    while (1) {
#if MOTOR_AUTOTEST_NO_PS2
        static int last_phase = -1;
        uint32_t phase = (k_uptime_get_32() / AUTOTEST_STEP_MS) % 3U;

        k_mutex_lock(&control_lock, K_FOREVER);

        if (phase == 0U) {
            zephyr_motor_set_speed(&motor_fl, AUTOTEST_SPEED);
            zephyr_motor_set_speed(&motor_fr, AUTOTEST_SPEED);
            zephyr_motor_set_speed(&motor_bl, AUTOTEST_SPEED);
            zephyr_motor_set_speed(&motor_br, AUTOTEST_SPEED);
        } else if (phase == 1U) {
            zephyr_motor_set_speed(&motor_fl, -AUTOTEST_SPEED);
            zephyr_motor_set_speed(&motor_fr, -AUTOTEST_SPEED);
            zephyr_motor_set_speed(&motor_bl, -AUTOTEST_SPEED);
            zephyr_motor_set_speed(&motor_br, -AUTOTEST_SPEED);
        } else {
            zephyr_motor_set_speed(&motor_fl, 0);
            zephyr_motor_set_speed(&motor_fr, 0);
            zephyr_motor_set_speed(&motor_bl, 0);
            zephyr_motor_set_speed(&motor_br, 0);
        }
        k_mutex_unlock(&control_lock);

        if ((int)phase != last_phase) {
            last_phase = (int)phase;
            if (phase == 0U) {
                LOG_INF("AUTOTEST: FORWARD");
            } else if (phase == 1U) {
                LOG_INF("AUTOTEST: BACKWARD");
            } else {
                LOG_INF("AUTOTEST: STOP");
            }
        }

        k_sleep(K_MSEC(50));
        continue;
#else
        bool success = ps2_read_gamepad(&ps2, false, 0);

        if (success) {
            k_mutex_lock(&control_lock, K_FOREVER);

            // кнопки крестовины
            if (ps2_button_pressed(&ps2, PSB_PAD_LEFT)) {
                LOG_INF("Move LEFT");
                change_mode(true);

                pos_fl += STEP_SIZE;
                pos_fr -= STEP_SIZE;
                pos_bl -= STEP_SIZE;
                pos_br += STEP_SIZE;

                update_target_positions();
            }

            if (ps2_button_pressed(&ps2, PSB_PAD_RIGHT)) {
                LOG_INF("Move RIGHT");
                change_mode(true);

                pos_fl -= STEP_SIZE;
                pos_fr += STEP_SIZE;
                pos_bl += STEP_SIZE;
                pos_br -= STEP_SIZE;

                update_target_positions();
            }

            if (ps2_button_pressed(&ps2, PSB_PAD_UP)) {
                LOG_INF("Move UP");
                change_mode(true);

                pos_fl += STEP_SIZE;
                pos_fr += STEP_SIZE;
                pos_bl += STEP_SIZE;
                pos_br += STEP_SIZE;

                update_target_positions();
            }

            if (ps2_button_pressed(&ps2, PSB_PAD_DOWN)) {
                LOG_INF("Move DOWN");
                change_mode(true);

                pos_fl -= STEP_SIZE;
                pos_fr -= STEP_SIZE;
                pos_bl -= STEP_SIZE;
                pos_br -= STEP_SIZE;

                update_target_positions();
            }

            // скоростной режим (стики)
            if (!position_mode) {
                // чтение стиков и преобразование их в скорости
                int val_lx = (int)ps2_analog(&ps2, PSS_LX) - 128;
                int val_ly = (int)ps2_analog(&ps2, PSS_LY) - 128;
                int val_rx = (int)ps2_analog(&ps2, PSS_RX) - 128;
                int val_ry = (int)ps2_analog(&ps2, PSS_RY) - 128;

                val_lx = val_lx * MAX_SPEED / 128;
                val_ly = val_ly * MAX_SPEED / 128;
                val_rx = val_rx * MAX_SPEED / 128;
                val_ry = val_ry * MAX_SPEED / 128;

                int duty_fr = val_ly + val_lx;
                int duty_fl = val_ly - val_lx;
                int duty_br = val_ly - val_lx;
                int duty_bl = val_ly + val_lx;

                // вращение
                duty_fr += val_ry - val_rx;
                duty_fl += val_ry + val_rx;
                duty_br += val_ry - val_rx;
                duty_bl += val_ry + val_rx;

                // ограничение
                if (duty_fr > MAX_SPEED) duty_fr = MAX_SPEED;
                if (duty_fr < -MAX_SPEED) duty_fr = -MAX_SPEED;
                if (duty_fl > MAX_SPEED) duty_fl = MAX_SPEED;
                if (duty_fl < -MAX_SPEED) duty_fl = -MAX_SPEED;
                if (duty_br > MAX_SPEED) duty_br = MAX_SPEED;
                if (duty_br < -MAX_SPEED) duty_br = -MAX_SPEED;
                if (duty_bl > MAX_SPEED) duty_bl = MAX_SPEED;
                if (duty_bl < -MAX_SPEED) duty_bl = -MAX_SPEED;

                zephyr_accelmotor_set_target_speed(&accel_fl, duty_fl);
                zephyr_accelmotor_set_target_speed(&accel_fr, duty_fr);
                zephyr_accelmotor_set_target_speed(&accel_bl, duty_bl);
                zephyr_accelmotor_set_target_speed(&accel_br, duty_br);
            }

            k_mutex_unlock(&control_lock);
        } else {
            k_mutex_lock(&control_lock, K_FOREVER);
            // остановка моторов при потери связи с PS2
            LOG_WRN("PS2 lost, stopping motors");
            change_mode(false);
            zephyr_accelmotor_set_target_speed(&accel_fl, 0);
            zephyr_accelmotor_set_target_speed(&accel_fr, 0);
            zephyr_accelmotor_set_target_speed(&accel_bl, 0);
            zephyr_accelmotor_set_target_speed(&accel_br, 0);
            k_mutex_unlock(&control_lock);
        }

        k_sleep(K_MSEC(50));
#endif
    }
}

static bool init_hardware(void) {
    LOG_INF("Initializing hardware...");

    const struct device *pwm_dev1 = DEVICE_DT_GET(DT_NODELABEL(pwm1));
    const struct device *pwm_dev2 = DEVICE_DT_GET(DT_NODELABEL(pwm2));
    const struct device *pwm_dev17 = DEVICE_DT_GET(DT_NODELABEL(pwm17));

    const struct device *gpio_dev_b = DEVICE_DT_GET(DT_NODELABEL(gpiob));
    const struct device *gpio_dev_e = DEVICE_DT_GET(DT_NODELABEL(gpioe));
    const struct device *gpio_dev_a = DEVICE_DT_GET(DT_NODELABEL(gpioa));
    const struct device *gpio_dev_c = DEVICE_DT_GET(DT_NODELABEL(gpioc));
    const struct device *gpio_dev_d = DEVICE_DT_GET(DT_NODELABEL(gpiod));

    // проверка устройств
    if (!device_is_ready(pwm_dev1)) {
        LOG_ERR("TIM1 not ready");
        return false;
    }
    if (!device_is_ready(pwm_dev2)) {
        LOG_ERR("TIM2 not ready");
        return false;
    }
    if (!device_is_ready(pwm_dev17)) {
        LOG_ERR("TIM17 not ready");
        return false;
    }
    if (!device_is_ready(gpio_dev_b)) {
        LOG_ERR("GPIOB not ready");
        return false;
    }
    if (!device_is_ready(gpio_dev_e)) {
        LOG_ERR("GPIOE not ready");
        return false;
    }
    if (!device_is_ready(gpio_dev_a)) {
        LOG_ERR("GPIOA not ready");
        return false;
    }
    if (!device_is_ready(gpio_dev_c)) {
        LOG_ERR("GPIOC not ready");
        return false;
    }
    if (!device_is_ready(gpio_dev_d)) {
        LOG_ERR("GPIOD not ready");
        return false;
    }

    /* L9110S wiring model:
     * - PWM pin goes to x-1A.
     * - DIR pin goes to x-1B.
     * Each channel therefore uses one PWM output and one GPIO direction pin.
     */

    // FL мотор: DIR PB13 (A-1B), PWM TIM1_CH1/PE9 (A-1A)
    zephyr_motor_init(&motor_fl, pwm_dev1, 1, gpio_dev_b, 13, false);
    zephyr_motor_set_min_duty(&motor_fl, MIN_DUTY);
    zephyr_motor_set_max_duty(&motor_fl, MAX_SPEED);

    // FR мотор: DIR PE10 (A-1B), PWM TIM1_CH3/PE13 (A-1A)
    zephyr_motor_init(&motor_fr, pwm_dev1, 3, gpio_dev_e, 10, false);
    zephyr_motor_set_min_duty(&motor_fr, MIN_DUTY);
    zephyr_motor_set_max_duty(&motor_fr, MAX_SPEED);

    // BL мотор: DIR PB8 (B-1B), PWM TIM2_CH1/PA0 (B-1A)
    zephyr_motor_init(&motor_bl, pwm_dev2, 1, gpio_dev_b, 8, false);
    zephyr_motor_set_min_duty(&motor_bl, MIN_DUTY);
    zephyr_motor_set_max_duty(&motor_bl, MAX_SPEED);

    // BR мотор: DIR PA5 (B-1B), PWM TIM17_CH1/PA7 (B-1A)
    zephyr_motor_init(&motor_br, pwm_dev17, 1, gpio_dev_a, 5, false);
    zephyr_motor_set_min_duty(&motor_br, MIN_DUTY);
    zephyr_motor_set_max_duty(&motor_br, MAX_SPEED);
    
    /* In standalone motor autotest we skip encoders/PID stack completely,
     * so motor actuation cannot fail due to sensor/control init.
     */
#if !MOTOR_AUTOTEST_NO_PS2
    if (!zephyr_encoder_init(&enc_fl, gpio_dev_b, 6)) return false;
    if (!zephyr_encoder_init(&enc_fr, gpio_dev_c, 6)) return false;
    if (!zephyr_encoder_init(&enc_bl, gpio_dev_e, 8)) return false;
    if (!zephyr_encoder_init(&enc_br, gpio_dev_d, 3)) return false;

    // инициализация доп. функций моторов
    zephyr_accelmotor_init(&accel_fl, &motor_fl, &enc_fl);
    zephyr_accelmotor_init(&accel_fr, &motor_fr, &enc_fr);
    zephyr_accelmotor_init(&accel_bl, &motor_bl, &enc_bl);
    zephyr_accelmotor_init(&accel_br, &motor_br, &enc_br);

    // настройка параметров
    zephyr_accelmotor_set_dt(&accel_fl, DT_MS);
    zephyr_accelmotor_set_dt(&accel_fr, DT_MS);
    zephyr_accelmotor_set_dt(&accel_bl, DT_MS);
    zephyr_accelmotor_set_dt(&accel_br, DT_MS);

    zephyr_accelmotor_set_run_mode(&accel_fl, PID_SPEED);
    zephyr_accelmotor_set_run_mode(&accel_fr, PID_SPEED);
    zephyr_accelmotor_set_run_mode(&accel_bl, PID_SPEED);
    zephyr_accelmotor_set_run_mode(&accel_br, PID_SPEED);

    zephyr_accelmotor_set_acceleration(&accel_fl, ACCEL);
    zephyr_accelmotor_set_acceleration(&accel_fr, ACCEL);
    zephyr_accelmotor_set_acceleration(&accel_bl, ACCEL);
    zephyr_accelmotor_set_acceleration(&accel_br, ACCEL);

    zephyr_accelmotor_set_max_speed(&accel_fl, MAX_FOLLOW_SPEED);
    zephyr_accelmotor_set_max_speed(&accel_fr, MAX_FOLLOW_SPEED);
    zephyr_accelmotor_set_max_speed(&accel_bl, MAX_FOLLOW_SPEED);
    zephyr_accelmotor_set_max_speed(&accel_br, MAX_FOLLOW_SPEED);

    zephyr_accelmotor_set_stopzone(&accel_fl, 5);
    zephyr_accelmotor_set_stopzone(&accel_fr, 5);
    zephyr_accelmotor_set_stopzone(&accel_bl, 5);
    zephyr_accelmotor_set_stopzone(&accel_br, 5);

    // ПИД коэф-ты
    accel_fl.kp = PID_P;
    accel_fr.kp = PID_P;
    accel_bl.kp = PID_P;
    accel_br.kp = PID_P;

    accel_fl.ki = PID_I;
    accel_fr.ki = PID_I;
    accel_bl.ki = PID_I;
    accel_br.ki = PID_I;

    accel_fl.kd = PID_D;
    accel_fr.kd = PID_D;
    accel_bl.kd = PID_D;
    accel_br.kd = PID_D;
#endif

    // инициализация PS2
#if MOTOR_AUTOTEST_NO_PS2
    LOG_WRN("PS2 init skipped: MOTOR_AUTOTEST_NO_PS2=1");
#else
    if (!ps2_init(&ps2, gpio_dev_c, 8, 12, 10, 11, false, false)) {
        LOG_ERR("Failed to initialize PS2 controller");
        return false;
    } else {
        LOG_INF("PS2 controller initialized successfully");
    }
#endif

    LOG_INF("Hardware initialization complete");
    return true;
}

K_THREAD_STACK_DEFINE(motor_stack, 2048);
K_THREAD_STACK_DEFINE(ps2_stack, 2048);
static struct k_thread motor_thread_data;
static struct k_thread ps2_thread_data;

void main(void) {
    LOG_INF("Starting Mecanum robot on NUCLEO-L496ZG");
    if (!init_hardware()) {
        LOG_ERR("Hardware init failed");
        return;
    }

    k_thread_create(&motor_thread_data, motor_stack, K_THREAD_STACK_SIZEOF(motor_stack),
                    motor_control_thread, NULL, NULL, NULL, 5, 0, K_NO_WAIT);
    k_thread_name_set(&motor_thread_data, "motor_ctrl");

    k_thread_create(&ps2_thread_data, ps2_stack, K_THREAD_STACK_SIZEOF(ps2_stack),
                    ps2_thread, NULL, NULL, NULL, 4, 0, K_NO_WAIT);
    k_thread_name_set(&ps2_thread_data, "ps2_ctrl");

    LOG_INF("System ready");
}
