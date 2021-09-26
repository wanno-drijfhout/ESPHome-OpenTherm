#pragma once

#include "esphome.h"
#include "OpenThermExt.h"

using namespace esphome;

/**
 * Represents an abstract OpenTherm data component to listen for a particular OpenTherm Data-Id.
 * 
 * There are two ways to get data into this component: through polling or by piggyback.
 * Every polling period, fresh data will be requested. If suitable data arrived by 
 * piggyback in between, that will be used immediately instead.
 * 
 * You should extend this and `Sensor` with `set_name`, `set_unit_of_measurement`,
 * `set_icon`, `set_accuracy_decimals`, `set_device_class`.
 * 
 * You should override `void processResponse()`; for example:
 * ```
 * void processResponse(uint16_t response) override
 * {
 *      publish_state(asF88(response));
 * }
 * ```
 */
class OpenThermDataComponent : public PollingComponent
{
public:
    OpenThermDataComponent(OpenThermExt *ot, OpenThermMessageID dataId)
        : PollingComponent(3000), ot_(ot), dataId_(dataId)
    {
        set_setup_priority(setup_priority::HARDWARE);
    }

    void setup() override
    {
        if (ot_ == nullptr || ot_->status == OpenThermStatus::NOT_INITIALIZED)
            status_set_error();
        needsUpdate_ = true;
        // TODO: register event to call updateData
    }

    /**
     * Polls
     */
    void update() override
    {
        // We may have gotten an appropriate response that the physical thermostat
        // already asked for (needsUpdate_ == false). In that case, we'll piggy-back
        // on that response once.
        if (needsUpdate_)
        {
            pollData();
        }

        needsUpdate_ = true;
    }

protected:
    /**
     * Interpret the response's 16-bit data field.
     * 
     * Implement this; use the 
     */
    virtual void updateData(uint16_t responseData) = 0;

    /**
     * Get the latest response data, interpreting the field as `u8` and `LB` (low-byte of the 16-bit data field).
     */
    uint8_t asU8_LB(uint16_t responseData) { return (uint8_t)(responseData & 0xff); }

    /**
     * Get the latest response data, interpreting the field as `u8` and `HB` (high-byte of the 16-bit data field).
     */
    uint8_t asU8_HB(uint16_t responseData) { return asU8_LB(responseData >> 8); }

    /**
     * Get the latest response data, interpreting the field as `s8` and `LB` (low-byte of the 16-bit data field).
     */
    int8_t asS8_LB(uint16_t responseData) { return (int8_t)(asU8_LB(responseData)); }

    /**
     * Get the latest response data, interpreting the field as `s8` and `HB` (high-byte of the 16-bit data field).
     */
    int8_t asS8_HB(uint16_t responseData) { return (int8_t)(asU8_HB(responseData)); }

    /**
     * Get the latest response data, interpreting the field as `u16`.
     */
    uint16_t asU16(uint16_t responseData) { return responseData; }

    /**
     * Get the latest response data, interpreting the field as `s16`.
     */
    int16_t asS16(uint16_t responseData) { return (int16_t)(asU16(responseData)); }

    /**
     * Get the latest response data, interpreting the field as `f8.8`.
     */
    float asF88(uint16_t responseData)
    {
        // 0x8000u corresponds to the Most Significant Bit (i.e., sign bit) of the 16-bit value
        // 256 corresponds to 2^8, which accounts for to the number of decimal places in f8.8 format.
        const float f = (responseData & 0x8000U) ? -(0x10000L - responseData) / 256.0f : responseData / 256.0f;
        return f;
    }

    /**
     * Writes an unsigned 16-bit integer (u16).
     * @return `true` if response is valid
     */
    bool writeAsU16(uint16_t requestData)
    {
        unsigned long request = ot_->buildRequest(OpenThermMessageType::WRITE_DATA, dataId_, requestData);
        unsigned long response = ot_->sendRequest(request);
        return ot_->isValidResponse(response);
    }

    /**
     * Writes an unsigned 8-bit integer (u8) and 8-bit integer (u8).
     * @return `true` if response is valid
     */
    bool writeAsU8(uint8_t requestDataHB, uint8_t requestDataLB)
    {
        return writeAsU16(((uint16_t)requestDataHB << 8) | (uint16_t)requestDataLB);
    }

    /**
     * Writes an signed fixed point value (f8.8).
     * @return `true` if response is valid
     */
    bool writeAsF88(float requestData)
    {
        // the multiplication by 256 (=2^8) shifts the fixed point value 8 bits
        return writeAsU16((uint16_t)(requestData * 256));
    }

private:
    OpenThermExt *ot_;
    OpenThermMessageID dataId_;
    bool needsUpdate_;

    void pollData()
    {
        uint32_t request = ot_->buildRequest(OpenThermRequestType::READ_DATA, dataId_, 0);
        uint32_t response = ot_->sendRequest(request);
        interpretResponse(response);
    }

    /**
     * Interpret a response relevant for the observed `dataId`.
     */
    void interpretResponse(uint32_t relevantResponse)
    {
        if (!ot_->isValidResponse(relevantResponse))
        {
            // We didn't get a valid response
            needsUpdate_ = true;
            status_set_error();
        }
        else if (ot_->getDataID(relevantResponse) != dataId_)
        {
            // We need the response to be *relevant* for this data ID
            ESP_LOGE("OpenThermDataComponent", "Bug? Received unexpected data ID [%s]; expected [%s]", ot_->getDataID(relevantResponse), dataId_);
        }
        else
        {
            // We got a valid response that is *relevant* for this data ID
            needsUpdate_ = false;
            status_clear_error();

            uint16_t responseData = asU16(relevantResponse);
            updateData(responseData);
        }
    }
};
