#include "sd_card.hpp"
#include <cstdio>
#include <pico/stdio.h>
#include <pico/stdio/driver.h>
#include <ff.h>
#include <hw_config.h>
#include "panic.hpp"

static FATFS fs;
static FIL stdout_file;
static bool initialized = false;

stdio_driver stdio_sd_driver {
    .out_chars = [](const char* buf, int len) {
        if (!initialized) return;
        UINT t;
        f_write(&stdout_file, buf, len, &t);
    },
    .out_flush = []() {
        if (!initialized) return;
        f_sync(&stdout_file);
    }
};

void sd_card::init() {
    if (initialized) return;
    FRESULT fr;
    fr = f_mount(&fs, "", 1);
    if (fr != FR_OK) {
        printf("f_mount error: %d\n", fr);
        panic(Error::SD_CARD_ERROR);
    }
    fr = f_open(&stdout_file, "stdout.txt", FA_OPEN_APPEND | FA_WRITE);
    if (!(fr == FR_OK || fr == FR_EXIST)) {
        printf("f_open error: %d\n", fr);
        panic(Error::SD_CARD_ERROR);
    }
    stdio_set_driver_enabled(&stdio_sd_driver, true);
    initialized = true;
}

void sd_card::deinit() {
    if (!initialized) return;
    stdio_set_driver_enabled(&stdio_sd_driver, false);
    f_close(&stdout_file);
    f_unmount("");
    initialized = false;
}

extern "C" {
static spi_t spi = {
    .hw_inst = spi1,
    .miso_gpio = 12,
    .mosi_gpio = 11,
    .sck_gpio = 10,
    .baud_rate = 125 * 1000 * 1000 / 4,
};
static sd_spi_if_t spi_if = {
    .spi = &spi,
    .ss_gpio = 13,
};
static sd_card_t sd = {
    .type = SD_IF_SPI,
    .spi_if_p = &spi_if,
};
size_t sd_get_num() { return 1; }
sd_card_t* sd_get_by_num(size_t num) { return num == 0 ? &sd : nullptr; }
}