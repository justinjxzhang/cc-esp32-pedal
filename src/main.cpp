#include <Arduino.h>
#include <Wire.h>
#include "main.h"
#include <SSD1306Wire.h>
#include <RotaryEncoderAdvanced.h>
#include <RotaryEncoderAdvanced.cpp>

#define TCAADDR 0x70 

BluetoothSerial BtSerial;
SSD1306Wire OLED(0x3C, GPIO_NUM_21, GPIO_NUM_22);
RotaryEncoderAdvanced<float> Rot(GPIO_NUM_33, GPIO_NUM_25, GPIO_NUM_32, 0.05, 0.0, 1.0);
float position = 0;
cc_device_t* device;

int assignmentCount = 0;
int currentAssignment = 0;

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

float controlValue = 1.0;

void tcaselect(uint8_t i) {
  if (i > 7) return;
  Wire.beginTransmission(TCAADDR);
  Wire.write(1 << i);
  Wire.endTransmission();
}

void updateNames(cc_assignment_t *assignment) {
  Serial.print("UPDATE-NAMES\tACTUATOR: ");
  Serial.print(assignment->actuator_id);
  Serial.print("\tASSIGNMENT_ID: ");
  Serial.print(assignment->id);
  Serial.print("\tLABEL: ");
  Serial.print(assignment->label.text);
  Serial.print("\tVALUE: ");
  Serial.println(assignment->value);

  assignmentCount += 1;
}

void updateControls(cc_assignment_t *assignment) {
  Serial.print("UPDATE-CONTROLS\tACTUATOR: ");
  Serial.print(assignment->actuator_id);
  Serial.print("\tASSIGNMENT_ID: ");
  Serial.print(assignment->id);
  Serial.print("\tLABEL: ");
  Serial.print(assignment->label.text);
  Serial.print("\tVALUE: ");
  Serial.println(assignment->value);
}

void unsetAssignment(int val) {
  Serial.print("UNASSIGN: ");
  Serial.println(val);
  assignmentCount -= 1;
}

void IRAM_ATTR rotISR() {
  Rot.readAB();
}

void IRAM_ATTR rotClickISR() {
  Rot.readPushButton();
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Begin Bluetooth");
  BtSerial.setTimeout(100);
  BtSerial.begin("ESP32 ControlChain");
  BtSerial.register_callback(btCallback);

  Rot.begin();
  attachInterrupt(digitalPinToInterrupt(GPIO_NUM_25), rotISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(GPIO_NUM_32), rotClickISR, FALLING);

  device = cc_device_new("ESP32 ControlChain", "http://www.github.com/");
  
  cc_actuator_config_t actuator_1_config;
  actuator_1_config.name = "Rotary 1";
  actuator_1_config.type = CC_ACTUATOR_CONTINUOUS;
  actuator_1_config.value = &position;
  actuator_1_config.min = 0.0;
  actuator_1_config.max = 1.0;
  actuator_1_config.supported_modes = CC_MODE_INTEGER | CC_MODE_REAL;
  actuator_1_config.max_assignments = 5;
  cc_actuator_t* actuator_1 = cc_actuator_new(&actuator_1_config);
  cc_device_actuator_add(device, actuator_1);

  // cc_actuator_config_t actuator_2_config;
  // actuator_2_config.name = "Rotary 2";
  // actuator_2_config.type = CC_ACTUATOR_CONTINUOUS;
  // actuator_2_config.value = &position;
  // actuator_2_config.min = 0.0;
  // actuator_2_config.max = 1.0;
  // actuator_2_config.supported_modes = CC_MODE_INTEGER | CC_MODE_REAL;
  // actuator_2_config.max_assignments = 5;
  // cc_actuator_t* actuator_2 = cc_actuator_new(&actuator_2_config);
  // cc_device_actuator_add(device, actuator_2);

  assignment_cb = updateNames;
  update_cb = updateControls;
  unassignment_cb = unsetAssignment;

  cc_init(responseCallback, eventsCallback);

  Wire.begin();
  
  tcaselect(6);
  OLED.init();
  OLED.setFont(ArialMT_Plain_10);
  OLED.clear();
  OLED.flipScreenVertically();
  OLED.setTextAlignment(TEXT_ALIGN_LEFT);
  OLED.drawString(0, 20, "Unassigned");
  OLED.display();

  tcaselect(7);
  OLED.init();
  OLED.setFont(ArialMT_Plain_10);
  OLED.clear();
  OLED.flipScreenVertically();
  OLED.setTextAlignment(TEXT_ALIGN_LEFT);
  OLED.drawString(0, 20, "Unassigned 2");
  OLED.display();
}


void loop() {
  if (position != Rot.getValue()) {
    position = Rot.getValue();
    Serial.println(position);
  }

  if (Rot.getPushButton() && assignmentCount > 0) {    
    cc_actuator_t* act = cc_device_actuator_get(device, 0);
    cc_actuator_next_assignment(act);
    cc_assignment_t* cAssignment = cc_get_current_assignment(act);
    Serial.printf("CHANGED TO: %s\n", cAssignment->label.text);

    tcaselect(6);
    OLED.clear();
    OLED.drawString(0, 20, cAssignment->label.text);
    OLED.display();
  }
  
  cc_process();
}

void btCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
  if (event == ESP_SPP_DATA_IND_EVT) {
    int available = BtSerial.available();
    if (available) {
      uint8_t data[available];
      BtSerial.readBytes(data, available);
      cc_data_t toParse = {data, available};
      cc_parse(&toParse);
    }
  }
}