#pragma once

#include "../esphome.h"

using namespace esphome;
using namespace esphome::output;
using namespace esphome::sensor;

/**
 * Represents the output of the integration's PID modulation algorithm.
 * 
 * Calls from external PID algorithm to `write_state` are expected.
 */
class PidRelativeModulationLevel : public FloatOutput, public Sensor, public Component
{
public:
    PidRelativeModulationLevel() : FloatOutput(), Sensor(), Component()
    {
        set_name("PID Relative Modulation Level");
        set_unit_of_measurement("%");
        set_icon("mdi:chart-bell-curve-cumulative");
        set_accuracy_decimals(0);
        set_device_class("power_factor");

        set_setup_priority(setup_priority::HARDWARE);
    }

    void setup() override
    {
    }

    void write_state(float state) override
    {
        publish_state(state);
    }

private:
};