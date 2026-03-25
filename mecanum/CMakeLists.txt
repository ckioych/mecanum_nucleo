cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(mecanum)

target_sources(app PRIVATE
    src/main.c
    src/zephyr_motor.c
    src/zephyr_encoder.c
    src/zephyr_accelmotor.c
    src/zephyr_ps2.c
)

target_include_directories(app PRIVATE src)