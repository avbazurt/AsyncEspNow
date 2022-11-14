#pragma once

#if CONFIG_IDF_TARGET_ESP32
#define CORE_ESP 0

#elif CONFIG_IDF_TARGET_ESP32S2
#define CORE_ESP 1

#elif CONFIG_IDF_TARGET_ESP32S3
#define CORE_ESP 0

#else
#error CORE no disponible para board objetivo!
#endif
