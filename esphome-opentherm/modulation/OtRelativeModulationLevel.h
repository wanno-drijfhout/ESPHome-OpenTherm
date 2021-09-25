#pragma once

#include "../esphome.h"
#include "../OpenThermExt.h"

using namespace esphome;
using namespace esphome::sensor;

/**
 * Represents the relative modulation level from the OpenTherm boiler.
 */
class OtRelativeModulationLevel : public PollingComponent, public Sensor
{
public:
    OtRelativeModulationLevel(OpenThermExt *ot)
        : PollingComponent(3000), Sensor(), ot_(ot)
    {
        set_name("Boiler Relative Modulation Level");
        set_unit_of_measurement("%");
        set_icon("mdi:chart-bell-curve-cumulative");
        set_accuracy_decimals(0);
        set_device_class("power_factor");

        set_setup_priority(setup_priority::HARDWARE);
    }

    void setup() override
    {
        if (ot_ == nullptr)
            status_set_error();
    }

    void update() override
    {
        auto state = ot_->getRelativeModulationLevel();
        publish_state(state);
        if (state == NAN)
            status_set_warning();
        else
            status_clear_warning();
    }

private:
    OpenThermExt *ot_;
};
