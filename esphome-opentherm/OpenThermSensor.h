#pragma once

#include "esphome.h"
#include "OpenThermExt.h"

using namespace esphome;
using namespace esphome::sensor;

/**
 * Represents an abstract OpenTherm Sensor.
 * 
 * You should implement the constructor with `set_name`, `set_unit_of_measurement`,
 * `set_icon`, `set_accuracy_decimals`, `set_device_class`.
 * 
 * You should override `void update()`; for example:
 * ```
 * auto state = otReadInt(OpenThermMessageID::Status);
 * publish_state(state.value_or(NAN));
 * ```
 */
class OpenThermSensor : public PollingComponent, public Sensor
{
public:
    OpenThermSensor(OpenThermExt *ot)
        : PollingComponent(3000), Sensor(), ot_(ot)
    {
        set_setup_priority(setup_priority::HARDWARE);
    }

    void setup() override
    {
        if (ot_ == nullptr || ot_->status == OpenThermStatus::NOT_INITIALIZED)
            status_set_error();
    }

    // void update() override
    // {
    //     auto state = otReadInt(/* insert */);
    //     publish_state(state);
    // }

protected:
    /**
     * Requests the value for the specified MessageID.
     */
    optional<uint16_t> otReadUInt(OpenThermMessageID messageId)
    {
        auto result = otRequestReadData(messageId);
        return result.has_value() ? make_optional(ot_->getUInt(result.value())) : optional<uint16_t>();
    }

    /**
     * Requests the value for the specificed MessageID.
     */
    optional<float> otReadFloat(OpenThermMessageID messageId)
    {
        auto result = otRequestReadData(messageId);
        return result.has_value() ? make_optional(ot_->getFloat(result.value())) : optional<float>();
    }

    /**
     * Writes a signed 16-bit integer (s16).
     * 
     * @return True if response is valid
     */
    bool otWriteInt(OpenThermMessageID messageId, uint16_t data)
    {
        unsigned long request = ot_->buildRequest(OpenThermMessageType::WRITE_DATA, messageId, data);
        unsigned long response = ot_->sendRequest(request);
        return ot_->isValidResponse(response);
    }

    /**
     * Writes an signed fixed point value (f8.8).
     * 
     * @return True if response is valid
     */
    bool otWriteFloat(OpenThermMessageID messageId, float data)
    {
        // the multiplication by 256 (=2^8) shifts the fixed point value 8 bits
	    unsigned int intData = (unsigned int)(data * 256);
        return otWriteInt(messageId, intData);
    }

private:
    OpenThermExt *ot_;

    optional<unsigned long> otRequestReadData(OpenThermMessageID messageId){
        unsigned long request = ot_->buildRequest(OpenThermRequestType::READ_DATA, messageId, 0);
        unsigned long response = ot_->sendRequest(request);
        return ot_->isValidResponse(response) ? make_optional(response) : optional<unsigned long>();
    }
    
};
