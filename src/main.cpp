#include <Arduino.h>
#include "main.h"

BluetoothSerial BtSerial;

void (*assignment_cb)(cc_assignment_t *) = 0;
void (*unassignment_cb)(int) = 0;
void (*update_cb)(cc_assignment_t *) = 0;

void responseCallback(void *arg) {
  cc_data_t *response = (cc_data_t *) arg;
  BtSerial.write(response->data, response->size);
}

void eventsCallback(void *arg) {
  cc_event_t *event = (cc_event_t *) arg;
  Serial.println("Events Callback");

  if (event->id == CC_EV_ASSIGNMENT) {
    cc_assignment_t *assignment = (cc_assignment_t *) event->data;

    if (assignment_cb)
        assignment_cb(assignment);
  } else if (event->id == CC_EV_UNASSIGNMENT) {
      int *act_id = (int *) event->data;
      int actuator_id = *act_id;

      if (unassignment_cb)
          unassignment_cb(actuator_id);
  } else if (event->id == CC_EV_UPDATE) {
      cc_assignment_t *assignment = (cc_assignment_t *) event->data;

      if (update_cb)
          update_cb(assignment);
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(1000);
  Serial.println("Begin Bluetooth");
  BtSerial.setTimeout(100);
  BtSerial.begin("ESP32 ControlChain");
  // BtSerial.register_callback(btCallback);

  cc_device_t* device = cc_device_new("ESP32 ControlChain", "http://www.github.com/");
  
  cc_actuator_config_t actuator_1_config;
  actuator_1_config.name = "Actuator1";
  actuator_1_config.type = CC_ACTUATOR_DISCRETE;
  cc_actuator_t* actuator_1 = cc_actuator_new(&actuator_1_config);
  cc_device_actuator_add(device, actuator_1);

  cc_init(responseCallback, eventsCallback);
}

void loop() {
  // put your main code here, to run repeatedly:
  // if (BtSerial.available()) {
  //   cc_process();
  // }
  int available = BtSerial.available();
  uint8_t data[available];
  BtSerial.readBytes(data, available);
  cc_data_t toParse = {data, available};
  cc_parse(&toParse);

  cc_process();
}

void doNothing(void *arg) {}
// void btCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
//   if (event == ESP_SPP_DATA_IND_EVT) {
//     uint8_t header[6];
//     BtSerial.readBytes(header, 6);

//     uint32_t dataSize = header[3] + (header[4] << 8);
//     uint8_t data[dataSize];
//     Serial.print("Size of data: ");
//     Serial.println(dataSize);
//     BtSerial.readBytes(data, dataSize);

//     uint8_t message[6 + dataSize];
//     for (int i = 0; i < 6; i++) {
//       message[i] = header[i];
//     }
//     for (int i = 0; i < dataSize; i++) {
//       message[i + 5] = data[i];
//     }

//     cc_data_t toParse = {message, 6 + dataSize};
//     cc_parse(&toParse);
//   }
// }
void btCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
  if (event == ESP_SPP_DATA_IND_EVT) {
    // uint8_t data[200];
    // uint8_t readSize = BtSerial.readBytes(data, 200);

    // cc_data_t toParse = {data, 6 + readSize};
    int available = BtSerial.available();
    uint8_t data[available];
    BtSerial.readBytes(data, available);
    cc_data_t toParse = {data, available};
    cc_parse(&toParse);
  }
}