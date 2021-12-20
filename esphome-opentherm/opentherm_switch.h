#pragma once

#include "esphome.h"

class OpenthermSwitch : public Switch {
  public:
    explicit OpenthermSwitch(bool state) {
      this->write_state(state);
    }

  protected:
    void write_state(bool state) override {
      // This will be called every time the user requests a state change.
      ESP_LOGD("opentherm_switch", "write_state");
      
      this->state = state;
      
      // Acknowledge new state by publishing it
      publish_state(state);
    }
};
