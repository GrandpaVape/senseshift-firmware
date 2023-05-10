// Override you configs in this file (Ctrl+Click)
#include "config/all.h"

#include <Arduino.h>
#include <Wire.h>

#include "senseshift.h"

#include <bh_utils.hpp>
#include <connection_bhble.hpp>
#include <output_writers/pwm.hpp>

#if defined(BATTERY_ENABLED) && BATTERY_ENABLED == true
#include "battery/adc_naive.hpp"
#endif

using namespace OH;
using namespace BH;

extern SenseShift App;
SenseShift* app = &App;

static const size_t bhLayoutSize = BH_LAYOUT_TACTVISOR_SIZE;
static const oh_output_point_t* bhLayout[bhLayoutSize] = BH_LAYOUT_TACTVISOR;

void setupMode() {
  // Configure PWM pins to their positions on the face
  auto faceOutputs = PlaneMapper_Margin::mapMatrixCoordinates<AbstractActuator>({
      // clang-format off
      {new PWMOutputWriter(32), new PWMOutputWriter(33), new PWMOutputWriter(25), new PWMOutputWriter(26)},
      // clang-format on
  });

  auto* face = new HapticPlane_Closest(faceOutputs);
  app->getHapticBody()->addComponent(OUTPUT_PATH_ACCESSORY, face);

  app->getHapticBody()->setup();

  uint8_t serialNumber[BH_SERIAL_NUMBER_LENGTH] = BH_SERIAL_NUMBER;
  ConnectionBHBLE_Config config = {
      .deviceName = BLUETOOTH_NAME,
      .appearance = BH_BLE_APPEARANCE,
      .serialNumber = serialNumber,
  };
  auto* bhBleConnection = new ConnectionBHBLE(config, [](std::string& value)->void {
    plainOutputTransformer(app->getHapticBody(), value, bhLayout, bhLayoutSize, OUTPUT_PATH_ACCESSORY);
  }, app);
  bhBleConnection->begin();

#if defined(BATTERY_ENABLED) && BATTERY_ENABLED == true
  auto* battery = new ADCNaiveBattery(36, { .sampleRate = BATTERY_SAMPLE_RATE }, &App, tskNO_AFFINITY);
  battery->begin();
#endif
}

void loopMode() {
  // Free up the Arduino loop task
  vTaskDelete(NULL);
}
