#include "esphome.h"
#include "esphome/components/sensor/sensor.h"
#include "OpenTherm.h"
#include "opentherm_switch.h"
#include "opentherm_climate.h"
#include "opentherm_binary.h"
#include "opentherm_output.h"
#include <math.h>

// --- Hardware configuration

// Pins to OpenTherm Master (virtual Thermostat, connects to real Boiler)
int boilerInPin = D2; 
int boilerOutPin = D1;

// Pins to OpenTherm Slave (virtual Boiler, connects to real Thermostat)
int thermostatInPin = D6;
int thermostatOutPin = D7;

// Determines if this OpenTherm instance is a gateway to a real connected thermostat (true) or standalone (false)
bool is_gateway = true;

// --- // Hardware configuration

OpenTherm boilerOT(boilerInPin, boilerOutPin, /*isSlave:*/ false);
OpenTherm thermostatOT(thermostatInPin, thermostatOutPin, /*isSlave:*/ true);

unsigned long boiler_last_response = 0;
unsigned long thermostat_last_request = 0;

ICACHE_RAM_ATTR void boilerHandleInterrupt() {
	boilerOT.handleInterrupt();
}

ICACHE_RAM_ATTR void thermostatHandleInterrupt() {
	thermostatOT.handleInterrupt();
}

class OpenthermComponent: public PollingComponent {
private:
  const char *TAG = "opentherm_component";
  OpenthermFloatOutput *pid_output_;
public:
  Switch *thermostat_switch = new OpenthermSwitch();
  Sensor *outside_temperature_sensor = new Sensor();
  Sensor *return_temperature_sensor = new Sensor();
  Sensor *boiler_modulation_sensor = new Sensor();
  Sensor *boiler_pressure_sensor = new Sensor();
  Sensor *central_heating_actual_temperature_sensor = new Sensor();
  Sensor *central_heating_target_temperature_sensor = new Sensor();
  OpenthermClimate *domestic_hot_water_climate = new OpenthermClimate();
  OpenthermClimate *central_heating_climate = new OpenthermClimate();
  BinarySensor *boiler_flame_sensor = new OpenthermBinarySensor(); 
  
  // Set 3 sec. to give time to read all sensors (and not appear in HA as not available)
  OpenthermComponent(): PollingComponent(3000) {
  }

  void setPidOutput(OpenthermFloatOutput *pid_output) { pid_output_ = pid_output; }

  void setup() override {
    // This will be called once to set up the component
    // think of it as the setup() call in Arduino
      ESP_LOGD("opentherm_component", "Setup");

      boilerOT.begin(boilerHandleInterrupt);
      if (is_gateway){
        ESP_LOGI("opentherm_component", "Operating as gateway");
        thermostatOT.begin(thermostatHandleInterrupt, [=](unsigned long request, OpenThermResponseStatus status) -> void {
          if (!thermostatOT.isValidRequest(request)) {
            ESP_LOGE("opentherm_component", "message ET%#010x received from thermostat (message type %s, data id %d)", request, thermostatOT.messageTypeToString(thermostatOT.getMessageType(request)), boilerOT.getDataID(request));
          } else {
            ESP_LOGD("opentherm_component", "message T%#010x received from thermostat", request);
          }
          thermostat_last_request = request;
        });
      }

      thermostat_switch->add_on_state_callback([=](bool state) -> void {
        ESP_LOGD ("opentherm_component", "thermostat_switch_on_state_callback %d", state);    
      });

      domestic_hot_water_climate->set_temperature_settings(5, 6, 0);
      domestic_hot_water_climate->setup();

      // Adjust HeatingWaterClimate depending on PID
      // central_heating_climate->set_supports_heat_cool_mode(this->pid_output_ != nullptr);
      // TODO: handle in thermostat_switch_on_state_callback?
      // TODO: make method for (this->pid_output_ != nullptr)
      central_heating_climate->set_supports_two_point_target_temperature(this->pid_output_ != nullptr);
      central_heating_climate->set_temperature_settings(0, 0, 30);
      central_heating_climate->setup();
  }

  float getOutsideTemperature() {
      unsigned long response = boilerOT.sendRequest(boilerOT.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::Toutside, 0));
      return boilerOT.isValidResponse(response) ? boilerOT.getFloat(response) : NAN;
  }

  float getReturnTemperature() {
      unsigned long response = boilerOT.sendRequest(boilerOT.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::Tret, 0));
      return boilerOT.isValidResponse(response) ? boilerOT.getFloat(response) : NAN;
  }
  
  float getDomesticHotWaterTemperature() {
      unsigned long response = boilerOT.sendRequest(boilerOT.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::Tdhw, 0));
      return boilerOT.isValidResponse(response) ? boilerOT.getFloat(response) : NAN;
  }

  bool setDomesticHotWaterTemperature(float temperature) {
	    unsigned int data = boilerOT.temperatureToData(temperature);
      unsigned long request = boilerOT.buildRequest(OpenThermRequestType::WRITE, OpenThermMessageID::TdhwSet, data);
      unsigned long response = boilerOT.sendRequest(request);
      return boilerOT.isValidResponse(response);
  }

  float getBoilerModulation() {
    unsigned long response = boilerOT.sendRequest(boilerOT.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::RelModLevel, 0));
    return boilerOT.isValidResponse(response) ? boilerOT.getFloat(response) : NAN;
  }

  float getBoilerPressure() {
    unsigned long response = boilerOT.sendRequest(boilerOT.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::CHPressure, 0));
    return boilerOT.isValidResponse(response) ? boilerOT.getFloat(response) : NAN;
  }
  
  // bool setTemperatureSetPointOverride(float temperature) {
  //   unsigned int data = boilerOT.temperatureToData(temperature);
	//   unsigned long request = boilerOT.buildRequest(OpenThermMessageType::WRITE_DATA, OpenThermMessageID::TrOverride, data);
  //   unsigned long response = boilerOT.sendRequest(request);
  //   return boilerOT.isValidResponse(response);
  // }

  void update() override {
    // TODO: split method in submethods

    if (is_gateway) {
      thermostatOT.process();

      // Forward last request
      auto response = boilerOT.sendRequest(thermostat_last_request);
      ESP_LOGD("opentherm_component", "message R%#010x sent to boiler", thermostat_last_request);
      if (response) {
          if (!boilerOT.isValidResponse(response)) {
            ESP_LOGE("opentherm_component", "message ET%#010x received from boiler (message type %s, data id %d)", response, boilerOT.messageTypeToString(boilerOT.getMessageType(response)), boilerOT.getDataID(response));
          } else {
            ESP_LOGD("opentherm_component", "message B%#010x received from boiler", response);
          }
      }
      boiler_last_response = response;

      // Forward last response
      thermostatOT.sendResponse(boiler_last_response);
      ESP_LOGD("opentherm_component", "message A%#010x sent to thermostat", boiler_last_response);

    } else {

      ESP_LOGD("opentherm_component", "update central_heating_climate: %s", climate_mode_to_string(central_heating_climate->mode));
      ESP_LOGD("opentherm_component", "update domestic_hot_water_climate: %s", climate_mode_to_string(domestic_hot_water_climate->mode));
      
      bool enable_central_heating = central_heating_climate->mode == ClimateMode::CLIMATE_MODE_HEAT;
      bool enable_domestic_hot_water = domestic_hot_water_climate->mode == ClimateMode::CLIMATE_MODE_HEAT;
      bool enable_cooling = false; // this boiler is for heating only

      boiler_last_response = boilerOT.setBoilerStatus(enable_central_heating, enable_domestic_hot_water, enable_cooling);
    }

    float central_heating_target_temperature = NAN;
    if (is_gateway) {
      // Override room thermostat set temperature

      // TODO: https://www.otgw.tclcode.com/firmware.html#dataids
      
      // central_heating_target_temperature = 20; //central_heating_climate->target_temperature;
      // ESP_LOGD("opentherm_component", "setTemperatureSetPointOverride at %f °C (override)", central_heating_target_temperature);
      // bool success = setTemperatureSetPointOverride(central_heating_target_temperature);
      // if (!success) {
      //     ESP_LOGD("opentherm_component", "setBoilerTemperature failed");
      // }
    } else {
      // Set temperature depending on room thermostat
      if (this->pid_output_ != nullptr) {
        float pid_output = pid_output_->get_state();
        if (pid_output != 0.0f) {
          central_heating_target_temperature = pid_output * (central_heating_climate->target_temperature_high - central_heating_climate->target_temperature_low) + central_heating_climate->target_temperature_low;
          ESP_LOGD("opentherm_component", "setBoilerTemperature at %f °C (from PID Output)", central_heating_target_temperature);    
        }
      }
      // TODO: rename "thermostat_switch": true disabled the PID, somehow...
      // TODO: same logic as gateway-operation?
      else if (thermostat_switch->state) {
        central_heating_target_temperature = central_heating_climate->target_temperature;
        ESP_LOGD("opentherm_component", "setBoilerTemperature at %f °C (from heating water climate)", central_heating_target_temperature);
      }
      // else {
      //   // If the room thermostat is off, set it to 10, so that the pump continues to operate
      //   central_heating_target_temperature = 10.0;
      //   ESP_LOGD("opentherm_component", "setBoilerTemperature at %f °C (default low value)", central_heating_target_temperature);
      // }

      if (!isnan(central_heating_target_temperature)) {
        boilerOT.setBoilerTemperature(central_heating_target_temperature);
      } else {
        ESP_LOGD("opentherm_component", "will not setBoilerTemperature (no target temperature)", central_heating_target_temperature);
      }

      // Set hot water temperature
      setDomesticHotWaterTemperature(domestic_hot_water_climate->target_temperature);
    }

    // Get Boiler status
    bool is_flame_on = boilerOT.isFlameOn(boiler_last_response);
    bool is_hot_water_active = boilerOT.isHotWaterActive(boiler_last_response);
    bool is_central_heating_active = boilerOT.isCentralHeatingActive(boiler_last_response);
    float outside_temperature = getOutsideTemperature();
    float return_temperature = getReturnTemperature();
    float boiler_modulation = getBoilerModulation();
    float boiler_pressure = getBoilerPressure();
    float central_heating_actual_temperature = boilerOT.getBoilerTemperature();
    float domestic_hot_water_temperature = getDomesticHotWaterTemperature();

    // Publish sensor values
    outside_temperature_sensor->publish_state(outside_temperature);
    return_temperature_sensor->publish_state(return_temperature);
    boiler_flame_sensor->publish_state(is_flame_on); 
    boiler_modulation_sensor->publish_state(boiler_modulation);
    boiler_pressure_sensor->publish_state(boiler_pressure);
    central_heating_actual_temperature_sensor->publish_state(central_heating_actual_temperature);
    central_heating_target_temperature_sensor->publish_state(central_heating_target_temperature);

    // Publish status of thermostat that controls hot water
    domestic_hot_water_climate->current_temperature = domestic_hot_water_temperature;
    domestic_hot_water_climate->action = is_hot_water_active ? (is_flame_on ? ClimateAction::CLIMATE_ACTION_HEATING : ClimateAction::CLIMATE_ACTION_IDLE) : ClimateAction::CLIMATE_ACTION_OFF;
    domestic_hot_water_climate->publish_state();
    
    // Publish status of thermostat that controls heating
    central_heating_climate->current_temperature = central_heating_actual_temperature;
    central_heating_climate->action = is_central_heating_active ? (is_flame_on ? ClimateAction::CLIMATE_ACTION_HEATING : ClimateAction::CLIMATE_ACTION_IDLE) : ClimateAction::CLIMATE_ACTION_OFF;
    central_heating_climate->publish_state();
  }

};