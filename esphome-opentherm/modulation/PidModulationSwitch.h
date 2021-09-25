#pragma once

#include "../esphome.h"

using namespace esphome;
using namespace esphome::switch_;

/**
 * Represents whether the integration's modulation is enabled or not.
 * 
 * If enabled, boiler target temperatures are overriden by the modulation output.
 * If disabled, boiler target temperatures are left untouched (i.e., as if the modulation output is 100%).
 */
class PidModulationSwitch : public Switch
{
public:
    explicit PidModulationSwitch(bool state) : Switch()
    {
        this->set_name("PID Modulation");
        this->set_icon("mdi:vector-curve");

        this->write_state(state);
    }

protected:
    void write_state(bool state) override
    {
        publish_state(state);
    }
};
