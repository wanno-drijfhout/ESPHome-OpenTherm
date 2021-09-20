#include "esphome.h"
#include "esphome/components/sensor/sensor.h"
#include "OpenTherm.h"
#include "opentherm_switch.h"
#include "opentherm_climate.h"
#include "opentherm_binary.h"
#include "opentherm_output.h"
#include <math.h>

// Pins to OpenTherm Master (Thermostat)
int mInPin = D2; 
int mOutPin = D1;
OpenTherm mOT(mInPin, mOutPin, /*isSlave:*/ false);

// Pins to OpenTherm Slave (Boiler)
int sInPin = D6;
int sOutPin = D7;
OpenTherm sOT(sInPin, sOutPin, /*isSlave:*/ true);
unsigned long slave_last_response = 0;

ICACHE_RAM_ATTR void mHandleInterrupt() {
	mOT.handleInterrupt();
}

ICACHE_RAM_ATTR void sHandleInterrupt() {
	sOT.handleInterrupt();
}

class OpenthermComponent: public PollingComponent {
private:
  const char *TAG = "opentherm_component";
  OpenthermFloatOutput *pid_output_;
public:
  Switch *thermostat_switch = new OpenthermSwitch();
  Sensor *outside_temperature_sensor = new Sensor();
  Sensor *return_temperature_sensor = new Sensor();
  Sensor *boiler_temperature_sensor = new Sensor();
  Sensor *central_heating_water_pressure_sensor = new Sensor();
  Sensor *boiler_modulation_sensor = new Sensor();
  Sensor *central_heating_target_temperature_sensor = new Sensor();
  OpenthermClimate *domestic_hot_water_climate = new OpenthermClimate();
  OpenthermClimate *central_heating_water_climate = new OpenthermClimate();
  BinarySensor *flame_sensor = new OpenthermBinarySensor(); 
  
  // Set 3 sec. to give time to read all sensors (and not appear in HA as not available)
  OpenthermComponent(): PollingComponent(3000) {
  }

  void setPidOutput(OpenthermFloatOutput *pid_output) { pid_output_ = pid_output; }

  void setup() override {
    // This will be called once to set up the component
    // think of it as the setup() call in Arduino
      ESP_LOGD("opentherm_component", "Setup");

      mOT.begin(mHandleInterrupt);
      sOT.begin(sHandleInterrupt, [=](unsigned long request, OpenThermResponseStatus status) -> void {
        ESP_LOGD("opentherm_component", "forwarding request from thermostat to boiler: %#010x", request);
        slave_last_response = mOT.sendRequest(request);
        if (slave_last_response) {
            ESP_LOGD("opentherm_component", "forwarding response from boiler to thermostat: %#010x", slave_last_response);
            sOT.sendResponse(slave_last_response);
        }
      });

      thermostat_switch->add_on_state_callback([=](bool state) -> void {
        ESP_LOGD ("opentherm_component", "thermostat_switch_on_state_callback %d", state);    
      });

      domestic_hot_water_climate->set_temperature_settings(5, 6, 0);
      domestic_hot_water_climate->setup();

      // Adjust HeatingWaterClimate depending on PID
      // central_heating_water_climate->set_supports_heat_cool_mode(this->pid_output_ != nullptr);
      central_heating_water_climate->set_supports_two_point_target_temperature(this->pid_output_ != nullptr);
      central_heating_water_climate->set_temperature_settings(0, 0, 30);
      central_heating_water_climate->setup();
  }

  float getOutsideTemperature() {
      unsigned long response = mOT.sendRequest(mOT.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::Toutside, 0));
      return mOT.isValidResponse(response) ? mOT.getFloat(response) : NAN;
  }

  float getReturnTemperature() {
      unsigned long response = mOT.sendRequest(mOT.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::Tret, 0));
      return mOT.isValidResponse(response) ? mOT.getFloat(response) : NAN;
  }
  
  float getDomesticHotWaterTemperature() {
      unsigned long response = mOT.sendRequest(mOT.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::Tdhw, 0));
      return mOT.isValidResponse(response) ? mOT.getFloat(response) : NAN;
  }

  bool setDomesticHotWaterTemperature(float temperature) {
	    unsigned int data = mOT.temperatureToData(temperature);
      unsigned long request = mOT.buildRequest(OpenThermRequestType::WRITE, OpenThermMessageID::TdhwSet, data);
      unsigned long response = mOT.sendRequest(request);
      return mOT.isValidResponse(response);
  }

  float getBoilerModulation() {
    unsigned long response = mOT.sendRequest(mOT.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::RelModLevel, 0));
    return mOT.isValidResponse(response) ? mOT.getFloat(response) : NAN;
  }

  float getCentralHeatingWaterPressure() {
    unsigned long response = mOT.sendRequest(mOT.buildRequest(OpenThermRequestType::READ, OpenThermMessageID::CHPressure, 0));
    return mOT.isValidResponse(response) ? mOT.getFloat(response) : NAN;
  }

  void update() override {

    // Process Thermostat Status
    sOT.process();

    ESP_LOGD("opentherm_component", "update central_heating_water_climate: %i", central_heating_water_climate->mode);
    ESP_LOGD("opentherm_component", "update domestic_hot_water_climate: %i", domestic_hot_water_climate->mode);
    
    bool enable_central_heating = central_heating_water_climate->mode == ClimateMode::CLIMATE_MODE_HEAT;
    bool enable_domestic_hot_water = domestic_hot_water_climate->mode == ClimateMode::CLIMATE_MODE_HEAT;
    bool enable_cooling = false; // this boiler is for heating only
    
    // Get/Set Boiler status
    auto response = slave_last_response != 0 ? slave_last_response : mOT.setBoilerStatus(enable_central_heating, enable_domestic_hot_water, enable_cooling);

    bool is_flame_on = mOT.isFlameOn(response);
    bool is_central_heating_active = mOT.isCentralHeatingActive(response);
    bool is_hot_water_active = mOT.isHotWaterActive(response);
    float return_temperature = getReturnTemperature();
    float domestic_hot_water_temperature = getDomesticHotWaterTemperature();

    // Set temperature depending on room thermostat
    float central_heating_target_temperature = NAN;
    if (this->pid_output_ != nullptr) {
      float pid_output = pid_output_->get_state();
      if (pid_output != 0.0f) {
        central_heating_target_temperature = pid_output * (central_heating_water_climate->target_temperature_high - central_heating_water_climate->target_temperature_low) + central_heating_water_climate->target_temperature_low;
        ESP_LOGD("opentherm_component", "setBoilerTemperature at %f °C (from PID Output)", central_heating_target_temperature);    
      }
    }
    else if (thermostat_switch->state) {
      central_heating_target_temperature = central_heating_water_climate->target_temperature;
      ESP_LOGD("opentherm_component", "setBoilerTemperature at %f °C (from heating water climate)", central_heating_target_temperature);
    }
    // else {
    //   // If the room thermostat is off, set it to 10, so that the pump continues to operate
    //   central_heating_target_temperature = 10.0;
    //   ESP_LOGD("opentherm_component", "setBoilerTemperature at %f °C (default low value)", central_heating_target_temperature);
    // }

    if (!isnan(central_heating_target_temperature)) {
      mOT.setBoilerTemperature(central_heating_target_temperature);
    } else {
      ESP_LOGD("opentherm_component", "will not setBoilerTemperature (no target temperature)", central_heating_target_temperature);
    }

    // Set hot water temperature
    setDomesticHotWaterTemperature(domestic_hot_water_climate->target_temperature);

    float boiler_temperature = mOT.getBoilerTemperature();
    float outside_temperature = getOutsideTemperature();
    float central_heating_water_pressure = getCentralHeatingWaterPressure();
    float boiler_modulation = getBoilerModulation();

    // Publish sensor values
    flame_sensor->publish_state(is_flame_on); 
    outside_temperature_sensor->publish_state(outside_temperature);
    return_temperature_sensor->publish_state(return_temperature);
    boiler_temperature_sensor->publish_state(boiler_temperature);
    central_heating_water_pressure_sensor->publish_state(central_heating_water_pressure);
    boiler_modulation_sensor->publish_state(boiler_modulation);
    central_heating_target_temperature_sensor->publish_state(central_heating_target_temperature);

    // Publish status of thermostat that controls hot water
    domestic_hot_water_climate->current_temperature = domestic_hot_water_temperature;
    domestic_hot_water_climate->action = is_hot_water_active ? (is_flame_on ? ClimateAction::CLIMATE_ACTION_HEATING : ClimateAction::CLIMATE_ACTION_IDLE) : ClimateAction::CLIMATE_ACTION_OFF;
    domestic_hot_water_climate->publish_state();
    
    // Publish status of thermostat that controls heating
    central_heating_water_climate->current_temperature = boiler_temperature;
    central_heating_water_climate->action = is_central_heating_active ? (is_flame_on ? ClimateAction::CLIMATE_ACTION_HEATING : ClimateAction::CLIMATE_ACTION_IDLE) : ClimateAction::CLIMATE_ACTION_OFF;
    central_heating_water_climate->publish_state();
  }

};