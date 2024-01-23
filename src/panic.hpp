#pragma once

enum class Error {
    WIFI_INIT_FAILED,
    WIFI_COULDNT_CONNECT,
    SENSOR_ERROR,
};

[[noreturn]] void panic(Error error);
