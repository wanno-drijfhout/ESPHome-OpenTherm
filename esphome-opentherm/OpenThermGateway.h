#pragma once

#include "esphome.h"
#include "OpenTherm.h"
#include "OpenThermConnection.h"

using namespace esphome;

class OpenThermGateway : public Component
{
public:
    OpenThermGateway(OpenThermConnection *boilerConnection, OpenThermConnection *thermostatConnection)
        : Component(), boilerConnection_(boilerConnection), thermostatConnection_(thermostatConnection)
    {
        set_setup_priority(setup_priority::HARDWARE);
    }

    void setup() override
    {
        thermostatConnection_->add_on_process_response_callback(std::bind(&OpenThermGateway::forwardRequestToBoiler, this, std::placeholders::_1));
    }

private:
    OpenThermConnection *boilerConnection_;
    OpenThermConnection *thermostatConnection_;

    void forwardRequestToBoiler(uint32_t request)
    {
        ESP_LOGD("OpenThermGateway", "forwarding request [%8x] to boiler", request);
        uint32_t response = boilerConnection_->sendRequest(request);
        ESP_LOGD("OpenThermGateway", "forwarding response [%8x] to thermostat", request);
        thermostatConnection_->sendResponse(response);
    }
};
