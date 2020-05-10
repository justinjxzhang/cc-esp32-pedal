#include "cc-slave/src/timer.h"
// #include "../lib/Ticker/Ticker.h"
#include <Arduino.h>
#include <Ticker.h>
#include "driver/timer.h"

void doNothing() {};

void (*g_callback)(void);
// void (*g_callback_2)(void *para) = {
//     g_callback();
// }

void g_callback_2(void *para) {
    g_callback();
}

Ticker Ticker1;
esp_timer_handle_t *timer;

void cc_timer_init(void (*callback)(void)) {
    g_callback = callback;
    // timer_config_t timerConfig;
    // timerConfig.divider = 80;
    // timerConfig.counter_en = false;
    // timer_init(TIMER_GROUP_0, TIMER_0, &timerConfig);
    // timer_isr_register(TIMER_GROUP_0, TIMER_0, g_callback_2, (void *) 1, ESP_INTR_FLAG_IRAM, NULL);

    // timerBegin
    // timer = esp_timer_init(timerConfig);
}

void cc_timer_set(uint32_t time_ms) {
    Ticker1.attach_ms(time_ms / 1000, g_callback);
}

void cc_delay_us(uint32_t time_us) {
    delayMicroseconds(time_us);
}