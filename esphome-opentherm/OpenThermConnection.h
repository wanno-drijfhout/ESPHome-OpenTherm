#pragma once

#include "esphome.h"
#include "OpenThermExt.h"
#include <iostream>

using namespace esphome;

/**
 * Represents a connection with an OpenTherm master or slave device.
 * 
 * Note: to `actAsSlave` means that the connected device is a master (i.e., thermostat).
 * To not `actAsSlave` means that the connected device is a slave (i.e., boiler).
 */
class OpenThermConnection : public Component, public Nameable
{
public:
    OpenThermConnection(int inPin, int outPin, bool actAsSlave)
        : Component(), inPin_(inPin), outPin_(outPin), actAsSlave_(actAsSlave)
    {
        ot_ = new OpenThermExt(inPin_, outPin_, actAsSlave_);

        set_setup_priority(setup_priority::IO);
        set_name(actAsSlave ? "thermostat" : "boiler");
        set_internal(true);
    }

    uint32_t hash_base() override { return 1116779545UL; }

    void setup() override
    {
        ot_ = new OpenThermExt(inPin_, outPin_, actAsSlave_);
        auto handleInterruptCallback2 = [this]() -> void { this->handleInterrupt(); };
        auto processResponseCallback2 = [this](unsigned long request, OpenThermResponseStatus status) -> void { this->processResponse(request, status); };
        ot_->begin(handleInterruptCallback2, processResponseCallback2);
    }

    void add_on_process_response_callback(std::function<void(uint32_t)> &&callback)
    {
        this->processResponseCallback_.add(std::move(callback));
    }

    uint32_t sendRequest(uint32_t request)
    {
        return ot_->sendRequest(request);
    }

    bool sendRequestAsync(uint32_t request)
    {
        return ot_->sendRequestAsync(request);
    }

    bool sendResponse(uint32_t response)
    {
        return ot_->sendResponse(response);
    }

    ICACHE_RAM_ATTR void handleInterrupt()
    {
        ot_->handleInterrupt();
    }

    // TODO: make private
    OpenThermExt *ot_;
private:
    int inPin_;
    int outPin_;
    bool actAsSlave_;

    CallbackManager<void(uint32_t)> processResponseCallback_;

    void processResponse(unsigned long request, OpenThermResponseStatus status)
    {
        if (!ot_->isValidResponse(request))
        {
            ESP_LOGE("OpenThermConnection", "message [%8x] received from %s (data id %d; message type %s)",
                     request, get_name().c_str(), ot_->getDataID(request), ot_->messageTypeToString(ot_->getMessageType(request)));
        }
        else
        {
            ESP_LOGV("OpenThermConnection", "message [%8x] received from %s (data id %d)",
                     request, get_name().c_str(), ot_->getDataID(request));
        }

        this->processResponseCallback_.call((uint32_t)request);
    }
};
