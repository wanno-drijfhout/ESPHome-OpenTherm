#pragma once

#include "../esphome.h"
#include "../OpenThermDataComponent.h"

using namespace esphome;
using namespace esphome::binary_sensor;

/**
 * Represents "CH mode [ CH not active, CH active ]" (ID 0, LB bit 1)
 */
class CentralHeatingMode : public OpenThermDataComponent, public BinarySensor
{
public:
    CentralHeatingMode(OpenThermExt *ot)
        : OpenThermDataComponent(ot, OpenThermMessageID::Status), BinarySensor()
    {
        set_name("Central Heating Mode");
        set_device_class("heat");
        // set_icon("mdi:water-boiler"); -- https://github.com/esphome/feature-requests/issues/360
    }

    void updateData(uint16_t response) override
    {
        bool active = asU8_LB(response) & (1 << 1);
        publish_state(active);
    }
};
