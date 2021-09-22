# ESPHome OpenTherm

This is an example of a integration with a OpenTherm boiler using [ESPHome](https://esphome.io/) and the [Ihor Melnyk](http://ihormelnyk.com/opentherm_adapter) or [DIYLESS](https://diyless.com/product/esp8266-thermostat-shield) OpenTherm Adapter 

## Installation
- Copy the content of this repository to your ESPHome folder
- Make sure the pin numbers are right, check the file `opentherm_component.h` in the `esphome-opentherm` folder.
- Edit the opentherm.yaml file:
    - Make sure the board and device settings are correct for your device
    - Set the sensor `entity_id` with the external temperature sensor's name from Home Assistant. (The ESPHome sensor name is `room_temperature_sensor`).
- Flash the ESP and configure in Home Assistant. It should be auto-discovered by the ESPHome Integration.

## System Design

### Climate control & sensors

This integration defines various climates, controls and sensors.

Items marked with (-) will be hidden by default, because they are included or abstracted by other sensors or climates. They exist only to debug and to prevent loss of information.

#### Room

Also known as "Room Thermostat" or "Air Thermostat".

- Climate
  - Room Temperature (from built-in sensor, specified sensor, or external thermostat `Tr`)
  - Room Target Temperature (from UI component, or `TrSet`, or `TrOverride`)
- Sensors
  - (-) Room Thermostat Temperature `Tr`
  - (-) Room Thermostat Target Temperature `TrSet`
  - (-) Room Remote Override Target Temperature `TrOverride`
  - Room Remote Override Function `RemoteOverrideFunction`
    - bit 0:  Manual change priority [disable/enable overruling remote setpoint by manual setpoint change ] 
    - bit 1:  Program change priority [disable/enable overruling remote setpoint by program setpoint change ]
- Controls

#### Hot Water

Also known as "Domestic Hot Water". Represents readily available warm tap water.

- Climate
  - Hot Water Temperature `Tdhw`
  - Hot Water Target Temperature `TdhwSet`
- Sensors
- Controls

#### Gateway (internal)

Also known as "PID Controller". Internal component not intended for direct user interaction. Controls and abstracts Boiler on behalf of Room (thermostat), possibly modulating and overriding values.

- Climate
  - Gateway Modulation (PID)
- Sensors
  - Relative Modulation Level (from `RelModLevel` or PID)
  - Relative Modulation Level Maximum (from `MaxRelModLevelSetting` or PID)
  - (-) Gateway Relative Modulation Level ("PID output")
- Controls
  - (-) Gateway Modulation Enabled (default: true if Thermostat Mode `master`)
  - (-) Gateway Modulation Autotune
  - (-) Gateway Thermostat Mode (`master` / `slave`)
- Additional functions
  - Pushing external sensor for room temperature to boiler
  - Pushing external sensor for weather temperature to boiler
  - Pushing external sensor for return temperature to boiler
  - Pushing date-time to boiler

#### Boiler

Also known as "Central Heating". Warms both Room and Hot Water.

- Climate
  - Boiler Flow Water Temperature `Tboiler`
  - Boiler Flow Water Target Temperature `Tset`
- Sensors
  - Boiler Return Water Temperature `Tret`
  - Boiler Flow Water Max Target Temperature `MaxTset`
  - Boiler Water Pressure `CHPressure`
  - Boiler Flame
  - Boiler Hot Water
  - (-) Boiler Fault
  - (-) Boiler Outside Temperature
  - (-) Boiler Relative Modulation Level (from `RelModLevel`)
  - (-) Boiler Relative Modulation Level Maximum (from `MaxRelModLevelSetting`)
- Controls

### Gateway operation modes

The gateway can operate as either of the following modes:

- **Slave**: mediate between *external* physical thermostat and boiler, allowing the gateway to override the desired Room temperature. (Requires OpenTherm master and slave boards to be physically connected as an "OpenTherm gateway".)
- **Master**: use *internal* thermostat as master to boiler, without any physical thermostat. (Requires OpenTherm master board _and_ external Room temperature sensor to be physically connected.)

Note that it is probably impossible to let an *external* physical thermostat simply observe the *internal* one.

### Modulation

The room climate thermostat can *modulate*, which heats the boiler gracefully at reduced power rather than going 100% or 0% to save energy and wear.

Modulation is implemented via a [PID controller](https://esphome.io/components/climate/pid.html). You may want to **autotune** modulation for your home, which provides new values for control parameters `kp`, `ki` and `kd`; these can be read as sensors or on the logging output.

Modulation can be disabled or enabled. In *gateway* mode, modulation would affect the `TrOverride`; if an external thermostat _is_ modulating already, modulation is unnecessary. In *master* mode, modulation would affect `TSet` and modulation is often useful.

## Future development

- [ ] Refactor code base (classes, naming, separate domain classes, implement named components)
- [ ] Investigate use-cases for "outside temperature" and "return temperature" ("weather compensator"?) -- Can we actually submit values to the boiler or only read them?
- [ ] Boiler should have single target_temperature; PID should add "pid_output" to current "room_temp"
- [ ] Investigate setting Date, Year, Day Of Week info
- [ ] Investigate reading setting boundaries from boiler (`TdhwSetUBTdhwSetLB`, `MaxTSetUBMaxTSetLB`, `MaxRelModLevelSetting`)
- [ ] Remote override room setpoint `TrOverride`: 0 or 1-30