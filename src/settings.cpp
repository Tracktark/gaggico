#include "settings.hpp"
#include <cstring>
#include <hardware/flash.h>
#include <pico/multicore.h>
#include "network.hpp"
#include "network/messages.hpp"
#include "network/impl/serde.hpp"
using namespace settings;

constexpr auto SETTINGS_FLASH_OFFSET = (1024 * 1024);
constexpr auto SETTINGS_MAGIC = "GAGGICO ";
constexpr u32 SETTINGS_VERSION = 4;
const u8* flash_settings = reinterpret_cast<const u8*>(XIP_BASE + SETTINGS_FLASH_OFFSET);

static Settings current_settings;

void load_default() {
    Settings default_settings;
    update(default_settings);
}

void settings::init() {
    const u8* curr_ptr = flash_settings;
    if (memcmp(SETTINGS_MAGIC, curr_ptr, strlen(SETTINGS_MAGIC))) { // Magic doesn't match
        load_default();
        return;
    }
    curr_ptr += strlen(SETTINGS_MAGIC);

    u32 flash_version;
    memcpy(&flash_version, curr_ptr, sizeof(SETTINGS_VERSION));
    if (flash_version != SETTINGS_VERSION) { // Version doesn't match
        load_default();
        return;
    }
    curr_ptr += sizeof(SETTINGS_VERSION);

    // Everything matches
    memcpy(&current_settings, curr_ptr, sizeof(Settings));
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

    memcpy(buf_ptr, &SETTINGS_VERSION, sizeof(SETTINGS_VERSION));
    buf_ptr += sizeof(SETTINGS_VERSION);

    memcpy(buf_ptr, &new_settings, sizeof(Settings));

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

    network::enqueue_message(SettingsGetMessage());
}

void Settings::write_data(u8*& ptr) const {
    write_struct(*this, ptr);
}

void Settings::read_data(u8*& ptr) {
    read_struct(*this, ptr);
}
