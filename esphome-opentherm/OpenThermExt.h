#pragma once

#include "OpenTherm.h"
#include <math.h>

// Workaround
// We cannot convert captured lambda handlers (e.g., with references to "this") to function pointers,
// which is what the class OpenTherm desired. The callbacks don't have any "tag" parameter either to
// help us distinguish between "this" instances. So, we'll hard-code we'd need two instances.
// The ideal solution is to refactor the function pointers into std::function<..>.
std::function<void(void)> _ot1_handleInterruptCallback = nullptr;
std::function<void(unsigned long, OpenThermResponseStatus)> _ot1_processResponseCallback = nullptr;
ICACHE_RAM_ATTR void _ot1_handleInterrupt()
{
  ESP_LOGD("OpenThermExt", "_ot1_handleInterrupt()");
  _ot1_handleInterruptCallback();
}
void _ot1_processResponse(unsigned long request, OpenThermResponseStatus status)
{
  ESP_LOGD("OpenThermExt", "_ot1_processResponse()");
  _ot1_processResponseCallback(request, status);
}

std::function<void(void)> _ot2_handleInterruptCallback = nullptr;
std::function<void(unsigned long, OpenThermResponseStatus)> _ot2_processResponseCallback = nullptr;
ICACHE_RAM_ATTR void _ot2_handleInterrupt()
{
  ESP_LOGD("OpenThermExt", "_ot2_handleInterrupt()");
  _ot2_handleInterruptCallback();
}
void _ot2_processResponse(unsigned long request, OpenThermResponseStatus status)
{
  ESP_LOGD("OpenThermExt", "_ot2_processResponse()");
  _ot2_processResponseCallback(request, status);
}

/**
 * This class should be subsumed into the OpenTherm library
 */
class OpenThermExt : public OpenTherm
{
public:
  OpenThermExt(int inPin = 4, int outPin = 5, bool isSlave = false) : OpenTherm(inPin, outPin, isSlave) {}

  void begin(std::function<void(void)> handleInterruptCallback, std::function<void(unsigned long, OpenThermResponseStatus)> processResponseCallback)
  {
    if (_ot1_handleInterruptCallback == nullptr)
    {
      _ot1_handleInterruptCallback = handleInterruptCallback;
      _ot1_processResponseCallback = processResponseCallback;
      ((OpenTherm *)this)->begin(&_ot1_handleInterrupt, processResponseCallback != nullptr ? &_ot1_processResponse : NULL);
    }
    else if (_ot2_handleInterruptCallback == nullptr)
    {
      _ot2_handleInterruptCallback = handleInterruptCallback;
      _ot2_processResponseCallback = processResponseCallback;
      ((OpenTherm *)this)->begin(&_ot2_handleInterrupt, processResponseCallback != nullptr ? &_ot2_processResponse : NULL);
    }
    else
    {
      ESP_LOGE("OpenThermExt", "Too many connections open! Hard-coded workaround for callbacks no longer working");
    }
  }

  void begin(std::function<void(void)> handleInterruptCallback)
  {
    begin(handleInterruptCallback, nullptr);
  }

  bool sendRequestAsync(unsigned long request)
  {
    // Note: typo "Aync"
    return sendRequestAync(request);
  }

  // OLD
  float getOutsideTemperature()
  {
    unsigned long response = sendRequest(buildRequest(OpenThermRequestType::READ, OpenThermMessageID::Toutside, 0));
    return isValidResponse(response) ? getFloat(response) : NAN;
  }

  float getReturnTemperature()
  {
    unsigned long response = sendRequest(buildRequest(OpenThermRequestType::READ, OpenThermMessageID::Tret, 0));
    return isValidResponse(response) ? getFloat(response) : NAN;
  }

  float getDomesticHotWaterTemperature()
  {
    unsigned long response = sendRequest(buildRequest(OpenThermRequestType::READ, OpenThermMessageID::Tdhw, 0));
    return isValidResponse(response) ? getFloat(response) : NAN;
  }

  bool setDomesticHotWaterTemperature(float temperature)
  {
    unsigned int data = temperatureToData(temperature);
    unsigned long request = buildRequest(OpenThermRequestType::WRITE, OpenThermMessageID::TdhwSet, data);
    unsigned long response = sendRequest(request);
    return isValidResponse(response);
  }

  float getRelativeModulationLevel()
  {
    unsigned long response = sendRequest(buildRequest(OpenThermRequestType::READ, OpenThermMessageID::RelModLevel, 0));
    return isValidResponse(response) ? getFloat(response) : NAN;
  }

  float getPressure()
  {
    unsigned long response = sendRequest(buildRequest(OpenThermRequestType::READ, OpenThermMessageID::CHPressure, 0));
    return isValidResponse(response) ? getFloat(response) : NAN;
  }

  // bool setTemperatureSetPointOverride(float temperature) {
  //   unsigned int data = temperatureToData(temperature);
  //   unsigned long request = buildRequest(OpenThermMessageType::WRITE_DATA, OpenThermMessageID::TrOverride, data);
  //   unsigned long response = sendRequest(request);
  //   return isValidResponse(response);
  // }
};