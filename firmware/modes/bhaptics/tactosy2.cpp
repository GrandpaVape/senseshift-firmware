// Override you configs in this file (Ctrl+Click)
#include "config/all.h"

#include <Arduino.h>
#include <Wire.h>

#include <utility.hpp>

#include "openhaptics.h"
#include "auto_output.h"

#include <connection_bhble.hpp>
#include "output_components/closest.h"
#include "output_writers/ledc.h"

#if defined(BATTERY_ENABLED) && BATTERY_ENABLED == true
#include "battery/adc_battery.h"
#endif

using namespace OH;
using namespace BH;

#pragma region bHaptics_trash

const uint16_t _bh_size_x = 3;
const uint16_t _bh_size_y = 2;

inline oh_output_point_t* make_point(oh_output_coord_t x, oh_output_coord_t y) {
  return getPoint(x, y, (oh_output_coord_t) (_bh_size_x - 1), (oh_output_coord_t) (_bh_size_y - 1));
}

oh_output_point_t* indexesToPoints[_bh_size_x * _bh_size_y] = {
    make_point(0, 0), make_point(1, 0), make_point(2, 0),
    make_point(0, 1), make_point(1, 1), make_point(2, 1),
};

void vestMotorTransformer(std::string& value) {
  for (size_t i = 0; i < (_bh_size_x * _bh_size_y); i++) {
    uint8_t byte = value[i];
    oh_output_data_t output_0;
    output_0.point = *indexesToPoints[i];
    output_0.intensity = map(byte, 0, 100, 0, UINT16_MAX);
    App.getOutput()->writeOutput(OUTPUT_PATH_ACCESSORY, output_0);
  }
}

#pragma endregion bHaptics_trash

void setupMode() {
  // Configure PWM pins to their positions on the forearm
  auto forearmOutputs = transformAutoOutput({
      // clang-format off
      {new LEDCOutputWriter(32), new LEDCOutputWriter(33), new LEDCOutputWriter(25)},
      {new LEDCOutputWriter(26), new LEDCOutputWriter(27), new LEDCOutputWriter(14)},
      // clang-format on
  });

  OutputComponent* forearm = new ClosestOutputComponent(forearmOutputs);

  App.getOutput()->addComponent(OUTPUT_PATH_ACCESSORY, forearm);

  uint8_t serialNumber[BH_SERIAL_NUMBER_LENGTH] = BH_SERIAL_NUMBER;
  AbstractConnection* bhBleConnection = new ConnectionBHBLE(BLUETOOTH_NAME, BH_BLE_APPEARANCE, serialNumber, vestMotorTransformer);
  App.setConnection(bhBleConnection);

#if defined(BATTERY_ENABLED) && BATTERY_ENABLED == true
  AbstractBattery* battery = new ADCBattery(33);
  App.setBattery(battery);
#endif
}
