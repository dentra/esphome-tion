[![Version][version-shield]][version]
[![License][license-shield]][license]
[![ESPHome release][esphome-release-shield]][esphome-release]
[![Open in Visual Studio Code][open-in-vscode-shield]][open-in-vscode]
[![Telegram][telegram-shield]][telegram]
[![Support author][donate-me-shield]][donate-me]
[![PayPal.Me][paypal-me-shield]][paypal-me]

[version-shield]: https://img.shields.io/static/v1?label=Version&message=2022.10.0&color=green
[version]: https://github.com/dentra/esphome-tion/releases/

[license-shield]: https://img.shields.io/static/v1?label=License&message=MIT&color=orange&logo=license
[license]: https://opensource.org/licenses/MIT

[esphome-release-shield]: https://img.shields.io/static/v1?label=ESPHome&message=2022.9&color=green&logo=esphome
[esphome-release]: https://github.com/esphome/esphome/releases/

[open-in-vscode-shield]: https://img.shields.io/static/v1?label=+&message=Open+in+VSCode&color=blue&logo=visualstudiocode
[open-in-vscode]: https://open.vscode.dev/dentra/esphome-tion

[telegram-shield]: https://img.shields.io/static/v1?label=+&message=Telegram&logo=telegram
[telegram]: https://t.me/esphome_tion

[donate-me-shield]: https://img.shields.io/static/v1?label=+&message=Donate
[donate-me]: https://www.tinkoff.ru/cf/3dZPaLYDBAI

[paypal-me-shield]: https://img.shields.io/static/v1?label=+&message=PayPal.Me&logo=paypal
[paypal-me]: https://paypal.me/dentra0

# Tion

Версия на русском языке доступна [здесь](README_ru.md).

This is a ESPHome component to control `Tion 4S`, `Tion Lite` and `Tion 3S` Breezers from ESP32 via BLE protocol and `Tion 4S` breezer via ingergation port with any ESP32/ESP8266 dongle.

At this moment the componet is build using climate platform and allows to control:

* On/Off
* Heater On/Off
* Target temperature
* Fan speed
* Buzzer
* Led (4S and Lite only)
* Inflow/Recirculation (4S only)
* Inflow/Recirculation/Mixed (3S only)
* Presets
* Boost preset time

And additionaly monitor:

* Temperature inside
* Temperature outside
* Heater power (4S and Lite only)
* Filter time left
* Filter warnout state (4S and Lite only)
* Boost time left
* Airflow counter (4S and Lite only)
* Current productivity (3S only)
* Version

## Firmware

You can find sample configuration files for [Tion 4S](tion-4s.yaml), [Tion Lite](tion-lt.yaml) and [Tion 3S](tion-3s.yaml) (they are self-descriptive).

* Download the necessary configuration for your breezer
* Modify `substitutions` section for your preferred values
* Change the set of packages to your needs
* Place the modified file in the ESPHome config directory
* Run the build and firmware upload of your configuration
* Add the device that appears to the Home Assistant

## Usage
After flashing firmware and before first run you need to enter your Breezer into the pairing mode (please follow the manual) and only then start ESP.

Additionally for `Tion 3S` you need to push `Pair` button.

>
> ### **WARNING: Everything that you do, you do at your own risk!**
>

## Roadmap

* Reset filters support.
* Support connection via hardware UART.

## Issue reporting

Feel free to open issues for bug reporting and feature requests. Will accept English and Russian language.

## Your thanks

If this project was useful to you or you want to support its further development you can always leave your thanks
via [Card2Card](https://www.tinkoff.ru/cf/3dZPaLYDBAI ) or [PayPal](https://paypal.me/dentra0 ).

## Commercial use

For questions of commercial use or custom development/improvement, please contact me by mail dennis.trachuk at gmail.com
