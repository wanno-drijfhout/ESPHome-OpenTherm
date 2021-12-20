#ifndef OpenThermExt_h
#define OpenThermExt_h

// TODO: split into .h and .cpp files
// TODO: reformat everything

#include "OpenTherm.h"

class OpenThermExt : public OpenTherm
{
public:
  OpenThermExt(int inPin = 4, int outPin = 5, bool isSlave = false): OpenTherm(inPin, outPin, isSlave){}

  float getOutsideTemperature() {
      unsigned long response = sendRequest(buildRequest(OpenThermRequestType::READ, OpenThermMessageID::Toutside, 0));
      return isValidResponse(response) ? getFloat(response) : NAN;
  }

  float getReturnTemperature() {
      unsigned long response = sendRequest(buildRequest(OpenThermRequestType::READ, OpenThermMessageID::Tret, 0));
      return isValidResponse(response) ? getFloat(response) : NAN;
  }
  
  float getDomesticHotWaterTemperature() {
      unsigned long response = sendRequest(buildRequest(OpenThermRequestType::READ, OpenThermMessageID::Tdhw, 0));
      return isValidResponse(response) ? getFloat(response) : NAN;
  }

  bool setDomesticHotWaterTemperature(float temperature) {
      unsigned int data = temperatureToData(temperature);
      unsigned long request = buildRequest(OpenThermRequestType::WRITE, OpenThermMessageID::TdhwSet, data);
      unsigned long response = sendRequest(request);
      return isValidResponse(response);
  }

  float getRelativeModulationLevel() {
    unsigned long response = sendRequest(buildRequest(OpenThermRequestType::READ, OpenThermMessageID::RelModLevel, 0));
    return isValidResponse(response) ? getFloat(response) : NAN;
  }

  float getPressure() {
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

#endif // OpenThermExt_h
