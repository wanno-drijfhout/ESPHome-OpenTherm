#pragma once

#include "esphome.h"

class OpenthermSwitch : public Switch {
  private:
    bool state_;
  public:
    explicit OpenthermSwitch(bool state) : state_(state) {}
    
    float get_state() const { return state_; }
  protected:
    void write_state(bool state) override {
      // This will be called every time the user requests a state change.
      ESP_LOGD("opentherm_switch", "write_state");
      state_ = state;

      // Acknowledge new state by publishing it
      publish_state(state);
    }
};
