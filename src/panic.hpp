#pragma once

enum class Error {
    WIFI_INIT_FAILED,
    WIFI_COULDNT_CONNECT,
    SENSOR_ERROR,
    COROUTINE_TOO_BIG,
    MESSAGE_QUEUE_FULL,
};

[[noreturn]] void panic(Error error);
