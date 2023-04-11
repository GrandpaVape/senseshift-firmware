#pragma once

#include "config/all.h"

#include <abstract_connection.hpp>
#include <haptic_body.hpp>
#include <events.hpp>

#if defined(BATTERY_ENABLED) && BATTERY_ENABLED == true
#include <abstract_battery.hpp>
#endif

#include <vector>

class OpenHaptics final : public OH::IEventDispatcher
{
 private:
  std::vector<const OH::IEventListener*> eventListeners{};
  OH::HapticBody* pHapticBody;

#if defined(BATTERY_ENABLED) && BATTERY_ENABLED == true
  OH::AbstractBattery* battery;
#endif

 public:
  OpenHaptics();

  OH::HapticBody* getHapticBody() {
    return this->pHapticBody;
  };

  void postEvent(const OH::IEvent* event) override;
  void addEventListener(const OH::IEventListener* listener) override;
};
