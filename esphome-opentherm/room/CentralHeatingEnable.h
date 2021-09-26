#pragma once

#include "../esphome.h"
#include "../OpenThermDataComponent.h"

using namespace esphome;
using namespace esphome::binary_sensor;

/**
 * Represents "CH enable [ CH is disabled, CH is enabled ]" (ID 0, HB bit 0)
 */
class CentralHeatingEnable : public OpenThermDataComponent, public BinarySensor
{
public:
    CentralHeatingEnable(OpenThermExt *ot)
        : OpenThermDataComponent(ot, OpenThermMessageID::Status), BinarySensor()
    {
        set_name("Central Heating Enable");
        set_device_class("heat");
        // set_icon("mdi:radiator-off"); -- https://github.com/esphome/feature-requests/issues/360
    }

    void updateData(uint16_t response) override
    {
        bool active = asU8_HB(response) & (1 << 0);
        // set_icon(active ? "mdi:radiator" : "mdi:radiator-disabled");
        publish_state(active);
    }
};
