[![Open in Visual Studio Code][open-in-vscode-shield]][open-in-vscode]
[![PayPal.Me][paypal-me-shield]][paypal-me]

[open-in-vscode-shield]: https://open.vscode.dev/badges/open-in-vscode.svg
[open-in-vscode]: https://open.vscode.dev/dentra/esphome-components

[paypal-me-shield]: https://img.shields.io/static/v1.svg?label=%20&message=PayPal.Me&logo=paypal
[paypal-me]: https://paypal.me/dentra0

# Tion

This is a ESPHome component to control `Tion 4s` and `Tion Lite` Breezers from ESP32 via BLE protocol.

At this moment the componet is build using climate platform and allows to control:

* On/Off
* Heater On/Off
* Fan speed
* Buzzer
* Led
* Inflow/Recirculation (4s Only)
* Boost Mode (4s Only)
* Boost Time (4s Only)

And additionaly monitor:

* Temperature inside
* Temperature outside
* Heater power
* Filter days left
* Filter warnout state
* Airflow counter
* Version

## Usage
> Everything that you do, you do at your own risk

### Sample configuration for Tion 4s
```yaml
external_components:
  - source: github://dentra/esphome-tion@2022.2.0

ota:
  on_begin:
    then:
      - lambda: id(ble_client_tion).set_enabled(false);

esp32_ble_tracker:

ble_client:
  - mac_address: $mac_tion
    id: ble_client_tion

climate:
  - platform: tion_4s
    ble_client_id: ble_client_tion
    name: "$name"
    buzzer:
      name: "$name Buzzer"
    led:
      name: "$name Led"
    recirculation:
      name: "$name Recirculation"
    temp_in:
      name: "$name Temp in"
    temp_out:
      name: "$name Temp out"
    heater_power:
      name: "$name Heater power"
    airflow_counter:
      name: "$name Airflow counter"
    filter_days_left:
      name: "$name Filter Days Left"
    filter_warnout:
      name: "$name Filter Warnout"
    boost_time:
      name: "$name Boost Time"
    version:
      name: "$name Version"
```

### Sample configuration for Tion Lite
```yaml
external_components:
  - source: github://dentra/esphome-tion@2022.2.0

ota:
  on_begin:
    then:
      - lambda: id(ble_client_tion).set_enabled(false);

esp32_ble_tracker:

ble_client:
  - mac_address: $mac_tion
    id: ble_client_tion

climate:
  - platform: tion_lt
    ble_client_id: ble_client_tion1
    name: "$name"
    buzzer:
      name: "$name Buzzer"
    led:
      name: "$name Led"
    temp_in:
      name: "$name Temp in"
    temp_out:
      name: "$name Temp out"
    heater_power:
      name: "$name Heater power"
    airflow_counter:
      name: "$name Airflow counter"
    filter_days_left:
      name: "$name Filter Days Left"
    filter_warnout:
      name: "$name Filter Warnout"
    version:
      name: "$name Version"
```

## Issue reporting

Feel free to open issues for bug reporting and feature requests. Will accept English and Russian language.

## Your thanks
If this project was useful to you, you can [buy me](https://paypal.me/dentra0) a Cup of coffee :)
