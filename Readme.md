# ESPHome OpenTherm Gateway

This integration connects to an OpenTherm boiler using [ESPHome](https://esphome.io/) and any of the following hardware:

- the adapter [Ihor Melnyk OpenTherm Adapter](http://ihormelnyk.com/opentherm_adapter);
- the adapter [DIYLESS ESP8266 Thermostat Shield](https://diyless.com/product/esp8266-thermostat-shield);
- the gateway [DIYLESS ESP8266 Opentherm Gateway](https://diyless.com/product/esp8266-opentherm-gateway).

Note that *adapters* do not support external physical thermostats hardware; you need a *gateway* if you want to retain usage of physical thermostats.

## Installation

- Copy the content of this repository to your ESPHome config folder
- Make sure the pin numbers are right, check the file `opentherm_component.h` in the `esphome-opentherm` folder.
- Edit the `opentherm.yaml` file:
    - Make sure the board and device settings are correct for your device
    - Set the sensor `entity_id` with the external temperature sensor's name from Home Assistant. (The ESPHome sensor name is `room_temperature_sensor`).
- Flash the ESP and configure in Home Assistant. It should be auto-discovered by the ESPHome Integration.

## Domains

There are *user* domains that represent what humans expect from a Central Heating Unit: **Room** and **Hot Water**.

There are *technical* domains that represent the available hardware: **Boiler**, **Thermostat**, **Integration**.

### Room

Represents the Room atmosphere/air/warmth in living spaces.

- Climate
  - Room Temperature (from built-in sensor, specified sensor, or external thermostat `Tr`)
  - Room Target Temperature (from UI component, or `TrSet`, or `TrOverride`)
- Sensors
  - Room Heating (active/inactive)

### Hot Water

Represents the readily available warm tap water ("Domestic Hot Water").

- Climate
  - Hot Water Temperature (`Tdhw`)
  - Hot Water Target Temperature (`TdhwSet`)
- Sensors
  - Hot Water Heating (active/inactive)
  
### Boiler (OpenTherm slave device)

Represents the "Central Heating Unit" warming Room and Hot Water.

- Climate
  - Boiler Flow Water Temperature `Tboiler`
  - Boiler Flow Water Target Temperature `Tset`
- Sensors
  - Boiler Return Water Temperature `Tret`
  - Boiler Flow Water Max Target Temperature `MaxTset`
  - Boiler Water Pressure `CHPressure`
  - Boiler Flame
  - (-) Boiler Fault (8-bit vector: Service Request, Low water, Gas/flame ,...)
  - (-) Boiler Outside Temperature
  - (-) Boiler Relative Modulation Level (from `RelModLevel`)
  - (-) Boiler Relative Modulation Level Maximum (from `MaxRelModLevelSetting`)

### Thermostat (optional OpenTherm master device)

Represents the external physical room thermostat. Only available in gateway mode.

- Room Temperature `Tr`
- Room Target Temperature `TrSet`
- Room Remote Override Target Temperature `TrOverride`
- Room Remote Override Function `RemoteOverrideFunction`
  - bit 0:  Manual change priority [disable/enable overruling remote setpoint by manual setpoint change ] 
  - bit 1:  Program change priority [disable/enable overruling remote setpoint by program setpoint change ]

### Integration (the ESPHome-OpenTherm unit)

Represents the software logic of the integration. Controls and abstracts Boiler on behalf of Room and Hot Water, possibly PID-modulating and overriding values. 

- Climate
  - Integration Modulation (PID)
- Sensors
  - Relative Modulation Level (from `RelModLevel` or PID)
  - Relative Modulation Level Maximum (from `MaxRelModLevelSetting` or PID)
  - (-) Integration Relative Modulation Level ("PID output")
- Controls
  - (-) Integration Modulation Enabled (default: true if Integration Mode `adapter`)
  - (-) Integration Modulation Autotune
  - (-) Integration Mode (`gateway` / `adapter`)
- Additional functions
  - Setting room temperature at boiler from external sensor
  - Setting weather temperature at boiler from external sensor
  - Setting return temperature at boiler from external sensor
  - Setting date-time and day-of-week at boiler

#### Integration Modes

The integration can operate in either of the following modes:

- **Gateway**: mediate between *external* physical thermostat and boiler, allowing the gateway to override the desired Room temperature. (Requires OpenTherm master and slave boards to be physically connected as an "OpenTherm gateway".)
- **Adapter**: use *internal* thermostat as master to boiler, without any physical thermostat. (Requires OpenTherm master board _and_ external Room temperature sensor to be physically connected.)

Note that it is probably impossible to let an *external* physical thermostat simply observe the *internal* one.

#### Integration Modulation

The room climate thermostat can *modulate*, which heats the boiler gracefully at reduced power rather than going 100% or 0% to save energy and wear.

Modulation is implemented via a [PID controller](https://esphome.io/components/climate/pid.html). You may want to **autotune** modulation for your home, which provides new values for control parameters `kp`, `ki` and `kd`; these can be read as sensors or on the logging output.

Modulation can be disabled or enabled. In *gateway* mode, modulation would affect the `TrOverride`; if an external thermostat _is_ modulating already, modulation is unnecessary. In *master* mode, modulation would affect `TSet` and modulation is often useful.

## Technical Architecture

This integration defines many sensors for the individual variables in the domain model. Each Sensor is named after its domain.

Sensors may observe other sensors to implement more complex logic (e.g., overriding temperatures, modulating, fallbacks).

## Future development

- [ ] Refactor code base (classes, naming, separate domain classes, implement named components)
- [ ] Investigate use-cases for "outside temperature" and "return temperature" ("weather compensator"?) -- Can we actually submit values to the boiler or only read them?
- [ ] Boiler should have single target_temperature; PID should add "pid_output" to current "room_temp"
- [ ] Investigate setting Date, Year, Day Of Week info
- [ ] Investigate reading setting boundaries from boiler (`TdhwSetUBTdhwSetLB`, `MaxTSetUBMaxTSetLB`, `MaxRelModLevelSetting`)
- [ ] Remote override room setpoint `TrOverride`: 0 or 1-30