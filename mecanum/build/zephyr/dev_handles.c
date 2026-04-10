#include <zephyr/device.h>
#include <zephyr/toolchain.h>

/* 1 : /soc/rcc@40021000:
 * Direct Dependencies:
 *   - (/soc)
 *   - (/clocks/pll)
 * Supported:
 *   - (/soc/adc@50040000)
 *   - (/soc/adc@50040100)
 *   - (/soc/adc@50040200)
 *   - (/soc/can@40006400)
 *   - (/soc/dac@40007400)
 *   - (/soc/dma@40020000)
 *   - (/soc/dma@40020400)
 *   - (/soc/flash-controller@40022000)
 *   - (/soc/i2c@40005400)
 *   - (/soc/i2c@40005800)
 *   - (/soc/i2c@40005c00)
 *   - (/soc/i2c@40008400)
 *   - (/soc/otgfs@50000000)
 *   - (/soc/quadspi@a0001000)
 *   - (/soc/rng@50060800)
 *   - /soc/rtc@40002800
 *   - (/soc/sdmmc@40012800)
 *   - /soc/serial@40004400
 *   - /soc/serial@40004800
 *   - (/soc/serial@40004c00)
 *   - (/soc/serial@40005000)
 *   - /soc/serial@40008000
 *   - (/soc/serial@40013800)
 *   - (/soc/spi@40003800)
 *   - (/soc/spi@40003c00)
 *   - (/soc/spi@40013000)
 *   - (/soc/timers@40000000)
 *   - (/soc/timers@40000400)
 *   - (/soc/timers@40000800)
 *   - (/soc/timers@40000c00)
 *   - (/soc/timers@40001000)
 *   - (/soc/timers@40001400)
 *   - (/soc/timers@40007c00)
 *   - (/soc/timers@40012c00)
 *   - (/soc/timers@40013400)
 *   - (/soc/timers@40014000)
 *   - (/soc/timers@40014400)
 *   - (/soc/timers@40014800)
 *   - (/soc/watchdog@40002c00)
 *   - /soc/pin-controller@48000000/gpio@48000000
 *   - /soc/pin-controller@48000000/gpio@48000400
 *   - /soc/pin-controller@48000000/gpio@48000800
 *   - /soc/pin-controller@48000000/gpio@48000c00
 *   - /soc/pin-controller@48000000/gpio@48001000
 *   - /soc/pin-controller@48000000/gpio@48001400
 *   - /soc/pin-controller@48000000/gpio@48001800
 *   - /soc/pin-controller@48000000/gpio@48001c00
 *   - /soc/pin-controller@48000000/gpio@48002000
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_rcc_40021000[] = { DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, 7, 8, 12, 6, 18, 19, 4, 9, 14, 11, 10, 17, 13, 3, 15, 5, 16, DEVICE_HANDLE_ENDS };

/* 2 : /soc/interrupt-controller@40010400:
 * Direct Dependencies:
 *   - (/soc)
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_interrupt_controller_40010400[] = { DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 3 : /soc/pin-controller@48000000/gpio@48002000:
 * Direct Dependencies:
 *   - (/soc/pin-controller@48000000)
 *   - /soc/rcc@40021000
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_pin_controller_48000000_S_gpio_48002000[] = { 1, DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 4 : /soc/pin-controller@48000000/gpio@48001c00:
 * Direct Dependencies:
 *   - (/soc/pin-controller@48000000)
 *   - /soc/rcc@40021000
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_pin_controller_48000000_S_gpio_48001c00[] = { 1, DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 5 : /soc/pin-controller@48000000/gpio@48001800:
 * Direct Dependencies:
 *   - (/soc/pin-controller@48000000)
 *   - /soc/rcc@40021000
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_pin_controller_48000000_S_gpio_48001800[] = { 1, DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 6 : /soc/pin-controller@48000000/gpio@48001400:
 * Direct Dependencies:
 *   - (/soc/pin-controller@48000000)
 *   - /soc/rcc@40021000
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_pin_controller_48000000_S_gpio_48001400[] = { 1, DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 7 : /soc/pin-controller@48000000/gpio@48001000:
 * Direct Dependencies:
 *   - (/soc/pin-controller@48000000)
 *   - /soc/rcc@40021000
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_pin_controller_48000000_S_gpio_48001000[] = { 1, DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 8 : /soc/pin-controller@48000000/gpio@48000c00:
 * Direct Dependencies:
 *   - (/soc/pin-controller@48000000)
 *   - /soc/rcc@40021000
 * Supported:
 *   - (/soc/spi@40013000)
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_pin_controller_48000000_S_gpio_48000c00[] = { 1, DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 9 : /soc/pin-controller@48000000/gpio@48000800:
 * Direct Dependencies:
 *   - (/soc/pin-controller@48000000)
 *   - /soc/rcc@40021000
 * Supported:
 *   - (/gpio_keys/button)
 *   - (/leds/led_1)
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_pin_controller_48000000_S_gpio_48000800[] = { 1, DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 10 : /soc/pin-controller@48000000/gpio@48000400:
 * Direct Dependencies:
 *   - (/soc/pin-controller@48000000)
 *   - /soc/rcc@40021000
 * Supported:
 *   - (/leds/led_2)
 *   - (/leds/led_3)
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_pin_controller_48000000_S_gpio_48000400[] = { 1, DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 11 : /soc/pin-controller@48000000/gpio@48000000:
 * Direct Dependencies:
 *   - (/soc/pin-controller@48000000)
 *   - /soc/rcc@40021000
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_pin_controller_48000000_S_gpio_48000000[] = { 1, DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 12 : /soc/rtc@40002800:
 * Direct Dependencies:
 *   - (/soc)
 *   - (/soc/interrupt-controller@e000e100)
 *   - /soc/rcc@40021000
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_rtc_40002800[] = { 1, DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 13 : /soc/serial@40004800:
 * Direct Dependencies:
 *   - (/soc)
 *   - (/soc/interrupt-controller@e000e100)
 *   - /soc/rcc@40021000
 *   - (/soc/pin-controller@48000000/usart3_rx_pd9)
 *   - (/soc/pin-controller@48000000/usart3_tx_pd8)
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_serial_40004800[] = { 1, DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 14 : /soc/serial@40008000:
 * Direct Dependencies:
 *   - (/soc)
 *   - (/soc/interrupt-controller@e000e100)
 *   - /soc/rcc@40021000
 *   - (/soc/pin-controller@48000000/lpuart1_rx_pg8)
 *   - (/soc/pin-controller@48000000/lpuart1_tx_pg7)
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_serial_40008000[] = { 1, DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 15 : /soc/serial@40004400:
 * Direct Dependencies:
 *   - (/soc)
 *   - (/soc/interrupt-controller@e000e100)
 *   - /soc/rcc@40021000
 *   - (/soc/pin-controller@48000000/usart2_rx_pd6)
 *   - (/soc/pin-controller@48000000/usart2_tx_pd5)
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_serial_40004400[] = { 1, DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 16 : /soc/timers@40014800/pwm:
 * Direct Dependencies:
 *   - (/soc/timers@40014800)
 *   - (/soc/pin-controller@48000000/tim17_ch1_pa7)
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_timers_40014800_S_pwm[] = { 1, DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 17 : /soc/timers@40014000/pwm:
 * Direct Dependencies:
 *   - (/soc/timers@40014000)
 *   - (/soc/pin-controller@48000000/tim15_ch1_pb14)
 * Supported:
 *   - (/pwmleds/red_pwm_led)
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_timers_40014000_S_pwm[] = { 1, DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 18 : /soc/timers@40000000/pwm:
 * Direct Dependencies:
 *   - (/soc/timers@40000000)
 *   - (/soc/pin-controller@48000000/tim2_ch1_pa0)
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_timers_40000000_S_pwm[] = { 1, DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 19 : /soc/timers@40012c00/pwm:
 * Direct Dependencies:
 *   - (/soc/timers@40012c00)
 *   - (/soc/pin-controller@48000000/tim1_ch1_pe9)
 *   - (/soc/pin-controller@48000000/tim1_ch2_pe11)
 *   - (/soc/pin-controller@48000000/tim1_ch3_pe13)
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_timers_40012c00_S_pwm[] = { 1, DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };
