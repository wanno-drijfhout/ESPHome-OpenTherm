#pragma once

#include "../esphome.h"
#include "../OpenThermSensor.h"

using namespace esphome;
using namespace esphome::sensor;

/**
 * Represents the relative modulation level from the OpenTherm boiler.
 */
class OtRelativeModulationLevel : public OpenThermSensor
{
public:
    OtRelativeModulationLevel(OpenThermExt *ot)
        : OpenThermSensor(ot)
    {
        set_name("Boiler Relative Modulation Level");
        set_unit_of_measurement("%");
        set_icon("mdi:chart-bell-curve-cumulative");
        set_accuracy_decimals(0);
        set_device_class("power_factor");
    }

    void update() override
    {
        auto state = otReadFloat(OpenThermMessageID::RelModLevel);
        publish_state(state.value_or(NAN));
    }
};
