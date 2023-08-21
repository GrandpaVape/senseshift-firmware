// Override you configs in this file (Ctrl+Click)
#include "config/all.h"

#include <Arduino.h>
#include <Wire.h>

#include "senseshift.h"

#include <senseshift/arduino/input/sensor/analog.hpp>
#include <senseshift/arduino/output/pwm.hpp>
#include <senseshift/battery/sensor.hpp>
#include <senseshift/bh/ble/connection.hpp>
#include <senseshift/bh/devices.hpp>
#include <senseshift/bh/encoding.hpp>

using namespace OH;
using namespace SenseShift;
using namespace SenseShift::Arduino::Output;
using namespace SenseShift::Arduino::Input;
using namespace SenseShift::Battery;
using namespace SenseShift::BH;
using namespace SenseShift::Body::Haptics;

extern SenseShift::SenseShift App;
SenseShift::SenseShift* app = &App;

static const size_t bhLayoutSize = BH_LAYOUT_TACTSUITX16_SIZE;
static const OutputLayout_t bhLayout[BH_LAYOUT_TACTSUITX16_SIZE] = BH_LAYOUT_TACTSUITX16;

// Ouput indices, responsible for x40 => x16 grouping
static const size_t layoutGroupsSize = BH_LAYOUT_TACTSUITX16_GROUPS_SIZE;
static const uint8_t layoutGroups[layoutGroupsSize] = BH_LAYOUT_TACTSUITX16_GROUPS;

void setupMode()
{
    // Configure PWM pins to their positions on the vest
    auto frontOutputs = PlaneMapper_Margin::mapMatrixCoordinates<VibroPlane::Actuator_t>({
      // clang-format off
      { new PWMOutputWriter(32), new PWMOutputWriter(33), new PWMOutputWriter(25), new PWMOutputWriter(26) },
      { new PWMOutputWriter(27), new PWMOutputWriter(14), new PWMOutputWriter(12), new PWMOutputWriter(13) },
      // clang-format on
    });
    auto backOutputs = PlaneMapper_Margin::mapMatrixCoordinates<VibroPlane::Actuator_t>({
      // clang-format off
      { new PWMOutputWriter(19), new PWMOutputWriter(18), new PWMOutputWriter(5), new PWMOutputWriter(17) },
      { new PWMOutputWriter(16), new PWMOutputWriter(4), new PWMOutputWriter(2), new PWMOutputWriter(15)  },
      // clang-format on
    });

    app->getHapticBody()->addTarget(Target::ChestFront, new VibroPlane_Closest(frontOutputs));
    app->getHapticBody()->addTarget(Target::ChestBack, new VibroPlane_Closest(backOutputs));

    app->getHapticBody()->setup();

    auto* bhBleConnection = new BLE::Connection(
      {
        .deviceName = BLUETOOTH_NAME,
        .appearance = BH_BLE_APPEARANCE,
        .serialNumber = BH_SERIAL_NUMBER,
      },
      [](std::string& value) -> void {
          Decoder::applyVestGrouped(app->getHapticBody(), value, bhLayout, layoutGroups);
      },
      app
    );
    bhBleConnection->begin();

#if defined(SENSESHIFT_BATTERY_ENABLED) && SENSESHIFT_BATTERY_ENABLED == true
    auto* battery = new BatterySensor(
      new NaiveBatterySensor(new AnalogSensor(36)),
      &App,
      { .sampleRate = SENSESHIFT_BATTERY_SAMPLE_RATE },
      { "ADC Battery", 4096, SENSESHIFT_BATTERY_TASK_PRIORITY, tskNO_AFFINITY }
    );
    battery->begin();
#endif
}

void loopMode()
{
    // Free up the Arduino loop task
    vTaskDelete(NULL);
}
