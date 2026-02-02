# System Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         HOME AUTOMATION SYSTEM                              │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│                          COMMUNICATION LAYER                                │
├─────────────────────────────────┬───────────────────────────────────────────┤
│  MQTT Client                    │  HTTP Client                              │
│  - Connect to broker            │  - Fetch hourly energy costs              │
│  - Subscribe to topics          │  - Get current cost                       │
│  - Publish commands             │  - API integration                        │
└─────────────────────────────────┴───────────────────────────────────────────┘
                                   │
                                   │
┌──────────────────────────────────┴──────────────────────────────────────────┐
│                           EVENT MANAGER                                     │
│                      (Thread-Safe Event Bus)                                │
│                                                                              │
│  Event Types:                                                                │
│  - TEMPERATURE_CHANGE    - ENERGY_COST_UPDATE                               │
│  - SOLAR_PRODUCTION      - ENERGY_CONSUMPTION                               │
│  - EV_CHARGER_STATUS     - APPLIANCE_CONTROL                                │
└──────────────────┬────────────────────────────────────────┬─────────────────┘
                   │                                        │
        ┌──────────┴────────┐                    ┌─────────┴──────────┐
        │                   │                    │                    │
┌───────▼──────┐    ┌───────▼──────┐    ┌───────▼──────┐    ┌───────▼──────┐
│   SENSORS    │    │  APPLIANCES  │    │   OPTIMIZER  │    │ CONTROLLERS  │
└──────────────┘    └──────────────┘    └──────────────┘    └──────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│                              SENSORS                                        │
├─────────────────────────────────────────────────────────────────────────────┤
│ ┌────────────────────┐  ┌────────────────────┐  ┌────────────────────┐   │
│ │ TemperatureSensor  │  │   EnergyMeter      │  │   SolarSensor      │   │
│ │ - Indoor/Outdoor   │  │ - Consumption (kW) │  │ - Production (kW)  │   │
│ │ - Current temp     │  │ - Real-time data   │  │ - Panel output     │   │
│ └────────────────────┘  └────────────────────┘  └────────────────────┘   │
│                                                                              │
│ ┌────────────────────┐                                                      │
│ │ EVChargerSensor    │                                                      │
│ │ - Charging status  │                                                      │
│ │ - Power draw (kW)  │                                                      │
│ └────────────────────┘                                                      │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│                            APPLIANCES                                       │
├─────────────────────────────────────────────────────────────────────────────┤
│ ┌────────────────┐  ┌────────────────┐  ┌────────────────┐                │
│ │    Heater      │  │ AirConditioner │  │     Light      │                │
│ │ - On/Off       │  │ - On/Off       │  │ - On/Off       │                │
│ │ - Target temp  │  │ - Target temp  │  │ - Brightness   │                │
│ │ - Power: 2 kW  │  │ - Power: 2.5kW │  │ - Power: 0.1kW │                │
│ └────────────────┘  └────────────────┘  └────────────────┘                │
│                                                                              │
│ ┌────────────────┐  ┌────────────────┐                                     │
│ │   Curtain      │  │   EVCharger    │                                     │
│ │ - Open/Close   │  │ - On/Off       │                                     │
│ │ - Position %   │  │ - Power adjust │                                     │
│ │ - Passive temp │  │ - Max: 11 kW   │                                     │
│ └────────────────┘  └────────────────┘                                     │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│                        ENERGY OPTIMIZER                                     │
│                    (Decision-Making Logic)                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  Decision Logic:                                                             │
│                                                                              │
│  1. EV Charging Optimization                                                │
│     IF cost > $0.15/kWh AND solar < current_charging_power                 │
│        THEN stop_charging()                                                 │
│     ELSE IF cost <= $0.10/kWh OR solar >= charging_power                   │
│        THEN resume_charging()                                               │
│                                                                              │
│  2. Temperature Control                                                     │
│     IF indoor_temp < target - 2°C                                           │
│        THEN turn_on_heater()                                                │
│     ELSE IF indoor_temp > target + 2°C                                      │
│        THEN turn_on_ac()                                                    │
│     ELSE IF within_range AND cost_is_high                                   │
│        THEN turn_off_hvac()                                                 │
│                                                                              │
│  3. Lighting Optimization                                                   │
│     IF cost_is_high AND solar_low                                           │
│        THEN reduce_brightness()                                             │
│                                                                              │
│  4. Passive Temperature Management                                          │
│     IF outdoor_temp > indoor_temp + 5°C                                     │
│        THEN close_curtains()  // Block heat                                 │
│     ELSE IF solar_available                                                 │
│        THEN open_curtains()   // Utilize solar heat                         │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│                         KEY FEATURES                                        │
├─────────────────────────────────────────────────────────────────────────────┤
│  ✓ Event-Driven Architecture    - Loose coupling, reactive system          │
│  ✓ Modular Design                - Easy to add sensors/appliances           │
│  ✓ Thread-Safe                   - Concurrent event handling                │
│  ✓ Cost Optimization             - Minimize energy costs                    │
│  ✓ Solar Utilization             - Leverage renewable energy                │
│  ✓ Comfort Maintenance           - Balance cost and user comfort            │
│  ✓ Extensible                    - Base classes for future additions        │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│                      EXAMPLE DECISION FLOW                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  1. Temperature Sensor detects indoor_temp = 18°C                           │
│     └─> Publishes TEMPERATURE_CHANGE event                                  │
│                                                                              │
│  2. Energy Optimizer receives event                                         │
│     └─> Calculates: target(22°C) - current(18°C) = 4°C difference          │
│     └─> Decision: Temperature too low (> 2°C below target)                  │
│                                                                              │
│  3. Optimizer checks energy cost from HTTP API = $0.12/kWh                  │
│     └─> Cost is moderate (between $0.10 and $0.15)                          │
│                                                                              │
│  4. Optimizer controls Heater                                               │
│     └─> Sends command: heater.turnOn()                                      │
│     └─> Heater activates (2 kW power consumption)                           │
│                                                                              │
│  5. Temperature rises to 22°C                                               │
│     └─> Optimizer detects target reached                                    │
│     └─> Sends command: heater.turnOff()                                     │
│     └─> Energy saved, comfort maintained                                    │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

## Data Flow

```
Sensors → Events → Event Manager → Optimizer → Appliances
   ↓                                    ↑
   └──────────── MQTT Topics ───────────┘

HTTP API → Optimizer → Decision Logic → Appliance Control
```

## Extensibility Example

To add a new sensor (e.g., Humidity Sensor):

1. Create `HumiditySensor.h` inheriting from `Sensor`
2. Add `HUMIDITY_CHANGE` to `EventType` enum
3. Implement `update()` method to publish humidity events
4. Subscribe to events in `EnergyOptimizer`
5. Add decision logic (e.g., control dehumidifier)

No changes needed to existing code!
