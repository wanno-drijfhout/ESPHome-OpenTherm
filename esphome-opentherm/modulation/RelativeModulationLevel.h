#pragma once

#include "../esphome.h"

using namespace esphome;
using namespace esphome::sensor;
using namespace esphome::switch_;

/**
 * Represents the gradual boiler control for improved efficiency and comfort.
 * 
 * Modulation can be done inside the boiler ("internal modulation"), by a physical
 * "modulating" thermostat (with a room temperature sensor), or by a "PID" algorithm
 * (provided by this integration, using an external room temperature sensor).
 * 
 * If your boiler or thermostat already does modulation, there is no need to enable this one.
 */
class RelativeModulationLevel : public Sensor, public Component
{
public:
    RelativeModulationLevel(Switch *pidModulationSwitch, Sensor *pidRelativeModulationLevel, Sensor *boilerRelativeModulationLevel)
        : modulationByIntegration_(pidModulationSwitch), pidRelativeModulationLevel_(pidRelativeModulationLevel), boilerRelativeModulationLevel_(boilerRelativeModulationLevel)
    {
        set_name("Relative Modulation Level");
        set_unit_of_measurement("%");
        set_icon("mdi:chart-bell-curve-cumulative");
        set_accuracy_decimals(0);
        set_device_class("power_factor");

        set_setup_priority(setup_priority::PROCESSOR);
    }

    void setup() override
    {
        modulationByIntegration_->add_on_state_callback([=](bool state) -> void { 
            ESP_LOGD("RelativeModulationLevel", "Changing definition of RelativeModulationLevel");
            update();
        });
        pidRelativeModulationLevel_->add_on_state_callback([=](float state) -> void {
            update();
        });
        boilerRelativeModulationLevel_->add_on_state_callback([=](float state) -> void {
            update();
        });
    }

private:
    Switch *modulationByIntegration_;
    Sensor *pidRelativeModulationLevel_;
    Sensor *boilerRelativeModulationLevel_;

    void update()
    {
        if (modulationByIntegration_->state)
            publish_state(pidRelativeModulationLevel_->get_state());
        else
            publish_state(boilerRelativeModulationLevel_->get_state());
    }
};