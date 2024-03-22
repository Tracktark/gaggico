#include "settings.hpp"
#include <cstring>
#include <hardware/flash.h>
#include <pico/multicore.h>
#include "network.hpp"
#include "network/messages.hpp"
#include "network/serde.hpp"
using namespace settings;

constexpr auto SETTINGS_FLASH_OFFSET = (1024 * 1024);
constexpr auto SETTINGS_MAGIC = "GAGGICO ";
constexpr u32 SETTINGS_VERSION = 3;
const u8* flash_settings = reinterpret_cast<const u8*>(XIP_BASE + SETTINGS_FLASH_OFFSET);

Settings current_settings;

void load_default() {
    Settings default_settings {
        .brew_temp = 97,
        .steam_temp = 150,
        .brew_pressure = 8,
        .preinfusion_pressure = 2,
        .preinfusion_time = 0,
        .brew_time = -1,
        .kp = 0.087,
        .ki = 0.00383,
        .kd = 0.49416,
    };
    update(default_settings);
}

void settings::init() {
    const u8* curr_ptr = flash_settings;
    if (memcmp(SETTINGS_MAGIC, curr_ptr, strlen(SETTINGS_MAGIC))) { // Magic doesn't match
        load_default();
        return;
    }
    curr_ptr += strlen(SETTINGS_MAGIC);

    u32 flash_version = *reinterpret_cast<const u32*>(curr_ptr);
    if (flash_version != SETTINGS_VERSION) { // Version doesn't match
        load_default();
        return;
    }
    curr_ptr += sizeof(SETTINGS_VERSION);

    // Everything matches
    current_settings = *reinterpret_cast<const Settings*>(curr_ptr);
}

const Settings& settings::get() {
    return current_settings;
}

void settings::update(Settings& new_settings) {
    current_settings = new_settings;

    static_assert(7 + sizeof(SETTINGS_VERSION) + sizeof(Settings) < FLASH_PAGE_SIZE, "Settings must be smaller than a page");
    u8 buffer[FLASH_PAGE_SIZE];
    u8* buf_ptr = buffer;

    memcpy(buf_ptr, SETTINGS_MAGIC, strlen(SETTINGS_MAGIC));
    buf_ptr += strlen(SETTINGS_MAGIC);

    *reinterpret_cast<u32*>(buf_ptr) = SETTINGS_VERSION;
    buf_ptr += sizeof(SETTINGS_VERSION);

    *reinterpret_cast<Settings*>(buf_ptr) = new_settings;

    u32 save = save_and_disable_interrupts();

    bool locked_out = false;
    if (multicore_lockout_victim_is_initialized(1 - get_core_num())) {
        locked_out = true;
        multicore_lockout_start_blocking();
    }

    flash_range_erase(SETTINGS_FLASH_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(SETTINGS_FLASH_OFFSET, buffer, FLASH_PAGE_SIZE);

    if (locked_out) {
        multicore_lockout_end_blocking();
    }
    restore_interrupts(save);

    network::send(SettingsGetMessage());
}

void Settings::write(u8*& ptr) const {
    write_val(ptr, brew_temp);
    write_val(ptr, steam_temp);
    write_val(ptr, brew_pressure);
    write_val(ptr, preinfusion_pressure);
    write_val(ptr, preinfusion_time);
    write_val(ptr, brew_time);
    write_val(ptr, kp);
    write_val(ptr, ki);
    write_val(ptr, kd);
}

void Settings::read(u8*& ptr) {
    read_val(ptr, brew_temp);
    read_val(ptr, steam_temp);
    read_val(ptr, brew_pressure);
    read_val(ptr, preinfusion_pressure);
    read_val(ptr, preinfusion_time);
    read_val(ptr, brew_time);
    read_val(ptr, kp);
    read_val(ptr, ki);
    read_val(ptr, kd);
}
