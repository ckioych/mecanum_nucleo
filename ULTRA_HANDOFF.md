# УЛЬТРА-HANDOFF: mecanum_nucleo

Дата: 2026-04-10

## 0) Кратко о цели
- Платформа на всенаправленных колесах (mecanum), 4 TT-мотора.
- Контроллер: Nucleo L496ZG.
- Драйверы: 2x L9110S (левый блок FL+BL, правый блок FR+BR).
- Финальное управление: PS2 gamepad.
- Текущий этап: диагностический режим из-за проблем левого драйвера/силовой части.

## 1) Среда и сборка
- Zephyr: 3.1.0
- Toolchain: Zephyr SDK 0.17.4
- Build flow: `cmake + make` (не `west build`)
- Flash: `west flash -r jlink`
- Логи: Serial Port Terminal (COM/tty ACM)

Команды:
- `cmake -DBOARD=nucleo_l496zg ..`
- `make -j4`
- `west flash -r jlink`

Если ccache мешает:
- `CCACHE_DISABLE=1 make -j4`
- `export CCACHE_TEMPDIR=/tmp/ccache-tmp`

## 2) Пины и аппаратная карта (по коду)
Моторы:
- FL: DIR pins PB13/PB14, PWM TIM1_CH1 (PE9)
- FR: DIR pins PE10/PE11, PWM TIM1_CH3 (PE13)
- BL: DIR pins PB8/PB9, PWM TIM2_CH1 (PA0)
- BR: DIR pins PA5/PA6, PWM TIM17_CH1 (PA7)

Датчики скорости (одноканальные ИК):
- FL: PB6 (D71)
- FR: PC6 (D16)
- BL: PE8 (D42)
- BR: PD3 (D55)

PS2 (битбанг GPIO):
- CLK: PC8
- CMD: PC12
- ATT/SEL: PC10
- DAT: PC11

## 3) Что уже исправлено в коде
### 3.1 PWM API (Zephyr 3.1)
- В `zephyr_motor.c` заменены вызовы `pwm_set_pulse_dt(...)` на `pwm_set(...)`.
- Добавлен include `<stdlib.h>` (для `abs` в motor module).

### 3.2 Линковка и математика в accelmotor
- В `zephyr_accelmotor.c` убраны зависимости от libc `fabs/abs`.
- Введены локальные функции `iabs32/fabs32`.
- Это убрало undefined references на линковке.

### 3.3 Потоки и порядок запуска
- Убрано автосоздание через `K_THREAD_DEFINE` для основной логики.
- Потоки создаются через `k_thread_create` только после успешного `init_hardware()`.

### 3.4 Синхронизация
- Добавлен mutex (`control_lock`) между motor/ps2 потоками для общих состояний.

### 3.5 Overlay/DTS
- Overlay приведен к рабочему состоянию для Zephyr 3.1.
- В `chosen` выставлены:
  - `zephyr,console = &usart3`
  - `zephyr,shell-uart = &usart3`
- Включен `pwm17` под TIM17 для BR.

### 3.6 Kconfig cleanup
- Из `prj.conf` удалены несовместимые символы:
  - `CONFIG_UART_CONSOLE_ON_DEV_NAME`
  - `CONFIG_TIMER`

## 4) Текущий режим (диагностика)
В `main.c` активен:
- `#define MOTOR_AUTOTEST_NO_PS2 1`

Эффект режима:
- PS2 отключен (инициализация пропущена).
- Контур PID/энкодеров отключен в `init_hardware()`.
- `motor_control_thread` не управляет моторами.
- Автотест делает прямое управление через `zephyr_motor_set_speed()`:
  - ~2 сек FORWARD
  - ~2 сек BACKWARD
  - ~2 сек STOP
  - цикл повторяется

## 5) Наблюдаемое состояние железа
- Правая сторона рабочая.
- Левый драйвер признан неисправным по симптомам C.C/перегрузки.
- При подключении проблемной стороны БП уходил в C.C, напряжение проседало циклически.
- Вывод: текущая проблема в силовой части, не в логике кода.

## 6) Почему код местами серый в VS Code
- Это inactive branch препроцессора (`#if/#else`) из-за `MOTOR_AUTOTEST_NO_PS2=1`.
- При `MOTOR_AUTOTEST_NO_PS2=0` активируется PS2-код, автотест-секция станет неактивной.

## 7) План после прихода новых компонентов
1. Проверить новый левый драйвер «на столе» (только VCC/GND, низкий лимит тока).
2. Подключить левый драйвер в схему без моторов.
3. Подключать моторы по одному, отслеживая C.C.
4. Запустить текущий автотест и убедиться, что стабильно работают все 4.
5. Перевести режим в PS2: `MOTOR_AUTOTEST_NO_PS2 -> 0`.
6. Пересобрать/прошить.
7. Проверить PS2 init и реакции на кнопки/стики.
8. Только после этого — финальная настройка PID/скоростей/мертвых зон.

## 8) Как должна работать PS2 логика (режим 0)
D-pad (position mode, шагами):
- LEFT: FL+, FR-, BL-, BR+
- RIGHT: FL-, FR+, BL+, BR-
- UP: все +
- DOWN: все -

Sticks (speed mode):
- трансляция через LX/LY
- вращение добавляется через RX/RY
- значения ограничиваются `MAX_SPEED`

Примечание:
- Поворот от RX/RY может быть неинтуитивным (обычно берут один RX); возможно понадобится тюнинг маппинга.

## 9) Файлы, которые точно изменялись ранее
- `mecanum/src/main.c`
- `mecanum/src/zephyr_motor.c`
- `mecanum/src/zephyr_accelmotor.c`
- `mecanum/prj.conf`
- `mecanum/nucleo_l496zg.overlay`

## 10) Риски/узкие места
- Основной риск: силовая часть.
- Отсутствие UART-логов не блокирует автотест, но усложняет диагностику.
- До замены левого драйвера нецелесообразно глубоко тюнить PS2/PID.

## 11) Контрольный чеклист
- `cmake` проходит без DTS/Kconfig ошибок.
- `make` доходит до `zephyr.elf`.
- `west flash -r jlink` успешно.
- В автотесте правый и левый блок крутят предсказуемо по циклу.
- Нет C.C на БП при штатном включении.

## 12) Точное действие для возврата в финальный сценарий
В `main.c` поменять:
- `#define MOTOR_AUTOTEST_NO_PS2 1`
на
- `#define MOTOR_AUTOTEST_NO_PS2 0`

После этого: rebuild + flash, затем верификация PS2 и кинематики.
