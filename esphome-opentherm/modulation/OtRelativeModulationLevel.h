#pragma once

#include "../esphome.h"
#include "../OpenThermDataComponent.h"

using namespace esphome;
using namespace esphome::sensor;

/**
 * Represents the relative modulation level from the OpenTherm boiler.
 */
class OtRelativeModulationLevel : public OpenThermDataComponent, public Sensor
{
public:
    OtRelativeModulationLevel(OpenThermExt *ot)
        : OpenThermDataComponent(ot, OpenThermMessageID::RelModLevel), Sensor()
    {
        set_name("Boiler Relative Modulation Level");
        set_unit_of_measurement("%");
        set_icon("mdi:chart-bell-curve-cumulative");
        set_accuracy_decimals(0);
        set_device_class("power_factor");
    }

    void updateData(uint16_t response) override
    {
        publish_state(asF88(response));
    }
};
