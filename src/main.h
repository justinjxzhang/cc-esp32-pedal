// #include "config.h"
#include "cc-slave/src/control_chain.h"
#include <BluetoothSerial.h>

void doNothing(void *arg);
void btCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param);