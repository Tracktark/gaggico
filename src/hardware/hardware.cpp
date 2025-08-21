#include "hardware.hpp"
#include <cstdio>
#include <cmath>
#include <hardware/gpio.h>
#include <hardware/spi.h>
#include <hardware/adc.h>
#include <hardware/pio.h>
#include <pico/sync.h>
#include <pico/time.h>
#include "hardware/thermal_runaway.hpp"
#include "hx711.pio.h"
#include "config.hpp"
#include "panic.hpp"
#include "psm.hpp"

using namespace hardware;

constexpr auto TEMP_CS_PIN = 5;
constexpr auto TEMP_SCK_PIN = 2;
constexpr auto TEMP_MISO_PIN = 4;
#define TEMP_SPI spi0
constexpr auto TEMP_READ_INTERVAL = 250;

constexpr auto PRESSURE_PIN = 26;

constexpr auto ZERO_CROSS_PIN = 7;
constexpr auto HEAT_DIM_PIN = 8;
constexpr auto PUMP_DIM_PIN = 9;

constexpr auto SOLENOID_PIN = 6;

constexpr auto LIGHT_PIN_BASE = 16;
constexpr auto SWITCH_PIN_BASE = 19;

#define SCALE_PIO pio0
constexpr auto SCALE_CLK_PIN = 22;
constexpr auto SCALE_DOUT_L_PIN = 14;
constexpr auto SCALE_DOUT_R_PIN = 15;
constexpr auto SCALE_SM_L = 0;
constexpr auto SCALE_SM_R = 1;
constexpr auto SCALE_MULT_L = 2189.f;
constexpr auto SCALE_MULT_R = -2273.f;
constexpr auto SCALE_TARE_STEPS = 5;

constexpr auto SD_SCK_PIN = 10;
constexpr auto SD_TX_PIN = 11;
constexpr auto SD_RX_PIN = 12;
constexpr auto SD_CS_PIN = 13;

static absolute_time_t switch_transition_time[3] = {nil_time};
static absolute_time_t next_temp_read_time = nil_time;
static critical_section_t temp_cs;
static PSM pump_psm(PUMP_DIM_PIN, 100);
static PSM heater_psm(HEAT_DIM_PIN, 100);
static ThermalRunawayCheck thermal_check;

static struct ScaleState {
    i32 offset_l;
    i32 offset_r;
    i32 tare_step = 0; // Setting to 0 means scales are tared automatically on startup
    i32 avg_l = 0;
    i32 avg_r = 0;
    i32 val_l = 0;
    i32 val_r = 0;
    float last_weight = NAN;
    bool connected = false;
} scale_state;

static void gpio_irq_handler(uint gpio, uint32_t event_mask) {
    if (gpio >= SWITCH_PIN_BASE && gpio < SWITCH_PIN_BASE + 3) {
        int which = gpio - SWITCH_PIN_BASE;
        switch_transition_time[which] = make_timeout_time_ms(20);
    }
    if (gpio == ZERO_CROSS_PIN && (event_mask & GPIO_IRQ_EDGE_RISE) > 0) {
        pump_psm.zero_crossing_handler();
        heater_psm.zero_crossing_handler();
    }
}


void hardware::init() {
    // Temp sensor
    spi_init(TEMP_SPI, 3'000'000);
    spi_set_format(TEMP_SPI, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(TEMP_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(TEMP_MISO_PIN, GPIO_FUNC_SPI);

    gpio_init(TEMP_CS_PIN);
    gpio_set_dir(TEMP_CS_PIN, GPIO_OUT);
    gpio_put(TEMP_CS_PIN, 1);

    critical_section_init(&temp_cs);

    // Pressure sensor
    adc_init();
    adc_gpio_init(PRESSURE_PIN);
    adc_select_input(PRESSURE_PIN - 26);
    static_assert(PRESSURE_PIN >= 26 && PRESSURE_PIN <= 29, "ADC only on pins 26-29");

    // Dimmers
    set_heater(0);
    set_pump(0);
    gpio_set_irq_enabled(ZERO_CROSS_PIN, GPIO_IRQ_EDGE_RISE, true);

    // Relay
    gpio_init(SOLENOID_PIN);
    gpio_set_dir(SOLENOID_PIN, true);
    set_solenoid(false);

    // Lights
    for (int i = 0; i < 3; ++i) {
        gpio_init(LIGHT_PIN_BASE + i);
        gpio_set_dir(LIGHT_PIN_BASE + i, true);
        gpio_put(LIGHT_PIN_BASE + 1, false);
    }
    // Switches
    for (int i = 0; i < 3; ++i) {
        int gpio = SWITCH_PIN_BASE + i;
        gpio_init(gpio);
        gpio_set_dir(gpio, false);
        gpio_pull_up(gpio);
        gpio_set_irq_enabled(gpio, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    }

    // Scales
    hx711_init(SCALE_PIO, SCALE_SM_L, SCALE_SM_R, SCALE_CLK_PIN,
               SCALE_DOUT_L_PIN, SCALE_DOUT_R_PIN);

    gpio_set_irq_callback(gpio_irq_handler);
    irq_set_enabled(IO_IRQ_BANK0, true);
}

void hardware::set_heater(float val) {
    heater_psm.set(val * 100);
}

void hardware::check_thermals() {
    if (thermal_check.has_fault(heater_psm.set_value, pump_psm.set_value, read_temp())) {
        panic(Error::THERMAL_RUNAWAY);
    }
}

void hardware::set_pump(float val) {
    pump_psm.set(val * 100);
}

u32 hardware::get_and_reset_pump_clicks() {
    u32 clicks = pump_psm.counter;
    pump_psm.counter = 0;
    return clicks;
}

void hardware::set_solenoid(bool active) {
    gpio_put(SOLENOID_PIN, active);
}

float hardware::read_temp() {
#ifdef DEBUG_NO_HARDWARE
    return 30;
#else
    critical_section_enter_blocking(&temp_cs);

    static float last_value = 0;

    if (!time_reached(next_temp_read_time)) {
        critical_section_exit(&temp_cs);
        return last_value;
    }
    next_temp_read_time = make_timeout_time_ms(TEMP_READ_INTERVAL);

    gpio_put(TEMP_CS_PIN, 0);
    asm volatile("nop \n nop \n nop");

    uint16_t data;
    spi_read16_blocking(TEMP_SPI, 0, &data, 1);
    data >>= 3;

    asm volatile("nop \n nop \n nop");
    gpio_put(TEMP_CS_PIN, 1);

    if (data == 0) {
        panic(Error::SENSOR_ERROR);
    }
    last_value = data * 0.25;
    if (last_value > 300) {
        panic(Error::SENSOR_ERROR);
    }
    critical_section_exit(&temp_cs);
    return last_value;
#endif
}

float hardware::read_pressure() {
    uint16_t value = adc_read();

    return ((float)value - 409.6f) / 273.07f;
}

float hardware::read_weight() {
    if (hx711_get(SCALE_PIO, SCALE_SM_L, SCALE_SM_R, &scale_state.val_l, &scale_state.val_r, &scale_state.connected)) {
        if (scale_state.tare_step >= 0) {
            scale_state.avg_l += scale_state.val_l;
            scale_state.avg_r += scale_state.val_r;
            scale_state.tare_step++;
        }
        if (scale_state.tare_step >= SCALE_TARE_STEPS) {
            scale_state.tare_step = -1;
            scale_state.offset_l = scale_state.avg_l / SCALE_TARE_STEPS;
            scale_state.offset_r = scale_state.avg_r / SCALE_TARE_STEPS;
        }
        float weight_l = (scale_state.val_l - scale_state.offset_l) / SCALE_MULT_L;
        float weight_r = (scale_state.val_r - scale_state.offset_r) / SCALE_MULT_R;
        scale_state.last_weight = weight_l + weight_r;
    }
    return scale_state.last_weight;
}

void hardware::scale_start_tare() {
    if (!scale_state.connected) return;
    scale_state.tare_step = 0;
    scale_state.avg_l = 0;
    scale_state.avg_r = 0;
}

void hardware::scale_tare_immediately() {
    if (!scale_state.connected) return;
    scale_state.offset_l = scale_state.val_l;
    scale_state.offset_r = scale_state.val_r;
}

bool hardware::is_scale_connected() { return scale_state.connected; }
bool hardware::is_scale_taring() {
    return scale_state.connected && scale_state.tare_step >= 0;
}

void hardware::set_light(Switch which, bool active) {
    gpio_put(LIGHT_PIN_BASE + which, active);
}

bool hardware::get_switch(Switch which) {
    static bool last_state[3] = {false};

    if (time_reached(switch_transition_time[which])) {
        last_state[which] = !gpio_get(SWITCH_PIN_BASE + which);
        switch_transition_time[which] = at_the_end_of_time;
    }

    return last_state[which];
}

bool hardware::is_power_just_pressed() {
    static bool last_pressed = false;
    bool curr_pressed = get_switch(Power);

    if (curr_pressed != last_pressed) {
        last_pressed = curr_pressed;
        return curr_pressed;
    }
    return false;
}
