#include "hardware.hpp"
#include <hardware/gpio.h>
#include <hardware/spi.h>
#include <hardware/adc.h>
#include <hardware/pio.h>
#include <pico/time.h>
#include "hardware/timer.h"
#include "pac.pio.h"
#include "config.hpp"

using namespace hardware;

constexpr auto TEMP_CS_PIN = 5;
constexpr auto TEMP_SCK_PIN = 2;
constexpr auto TEMP_MISO_PIN = 4;
#define TEMP_SPI spi0 
constexpr auto TEMP_READ_INTERVAL = 250;

constexpr auto PRESSURE_PIN = 26;

#define DIMMER_PIO pio0
constexpr auto ZERO_CROSS_PIN = 10;
constexpr auto HEAT_DIM_PIN = 11;
constexpr auto PUMP_DIM_PIN = 12;
constexpr auto PUMP_PIO_SM = 0;
constexpr auto HEAT_PIO_SM = 1;

constexpr auto SOLENOID_PIN = 9;

constexpr auto LIGHT_PIN_BASE = 16;
constexpr auto SWITCH_PIN_BASE = 19;


void hardware::init() {
    // Temp sensor
    spi_init(TEMP_SPI, 3'000'000);
    spi_set_format(TEMP_SPI, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(TEMP_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(TEMP_MISO_PIN, GPIO_FUNC_SPI);

    gpio_init(TEMP_CS_PIN);
    gpio_set_dir(TEMP_CS_PIN, GPIO_OUT);
    gpio_put(TEMP_CS_PIN, 1);

    // Pressure sensor
    adc_init();
    adc_gpio_init(PRESSURE_PIN);
    adc_select_input(PRESSURE_PIN - 26);
    static_assert(PRESSURE_PIN >= 26 && PRESSURE_PIN <= 29, "ADC only on pins 26-29");

    // Dimmers
    uint offset = pio_add_program(DIMMER_PIO, &pac_program);
    pac_pio_init(DIMMER_PIO, HEAT_PIO_SM, offset, HEAT_DIM_PIN, ZERO_CROSS_PIN);
    pac_pio_init(DIMMER_PIO, PUMP_PIO_SM, offset, PUMP_DIM_PIN, ZERO_CROSS_PIN);
    set_heater(0);
    set_pump(0);

    // Relay
    gpio_init(SOLENOID_PIN);
    gpio_set_dir(SOLENOID_PIN, true);
    gpio_put(SOLENOID_PIN, 1);
    set_solenoid(false);

    // Lights
    for (int i = 0; i < 3; ++i) {
        gpio_init(LIGHT_PIN_BASE + i);
        gpio_set_dir(LIGHT_PIN_BASE + i, true);
        gpio_put(LIGHT_PIN_BASE + 1, false);
    }
    // Switches
    for (int i = 0; i < 3; ++i) {
        gpio_init(SWITCH_PIN_BASE + i);
        gpio_set_dir(SWITCH_PIN_BASE + i, false);
        gpio_pull_up(SWITCH_PIN_BASE + i);
    }
}

void hardware::set_heater(float val) {
    pac_pio_set(DIMMER_PIO, HEAT_PIO_SM, MAINS_FREQUENCY_HZ, val);
}

void hardware::set_pump(float val) {
    pac_pio_set(DIMMER_PIO, PUMP_PIO_SM, MAINS_FREQUENCY_HZ, val);
}

void hardware::set_solenoid(bool active) {
    gpio_put(SOLENOID_PIN, active);
}

float hardware::read_temp() {
    static absolute_time_t next_read_time = nil_time;
    static float last_value = 0;

    if (!time_reached(next_read_time)) {
        return last_value;
    }
    next_read_time = make_timeout_time_ms(TEMP_READ_INTERVAL);

    gpio_put(TEMP_CS_PIN, 0);
    asm volatile("nop \n nop \n nop");

    uint16_t data;
    spi_read16_blocking(TEMP_SPI, 0, &data, 1);
    data >>= 3;

    asm volatile("nop \n nop \n nop");
    gpio_put(TEMP_CS_PIN, 1);

    last_value = data * 0.25;
    return last_value;
}

float hardware::read_pressure() {
    uint16_t value = adc_read();

    return (value - 410) / 273.f;
}

void hardware::set_light(Switch which, bool active) {
    gpio_put(LIGHT_PIN_BASE + which, active);
}

bool hardware::get_switch(Switch which) {
    return !gpio_get(SWITCH_PIN_BASE + which);
}

bool hardware::is_power_just_pressed() {

    static bool curr_pressed = false;
    static absolute_time_t next_press_time = nil_time;

    if (!curr_pressed && get_switch(Power) && time_reached(next_press_time)) {
        next_press_time = make_timeout_time_ms(200);
        curr_pressed = true;
        return true;
    }
    if (curr_pressed && !get_switch(Power)) {
        curr_pressed = false;
    }
    return false;
}

