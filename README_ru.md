[![Version][version-shield]][version]
[![License][license-shield]][license]
[![ESPHome release][esphome-release-shield]][esphome-release]
[![Open in Visual Studio Code][open-in-vscode-shield]][open-in-vscode]
[![Telegram][telegram-shield]][telegram]
[![Support author][donate-me-shield]][donate-me]
[![PayPal.Me][paypal-me-shield]][paypal-me]

[version-shield]: https://img.shields.io/static/v1?label=Version&message=2022.10.1&color=green
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

English version of this page is available [here](README.md).

Компонет ESPHome для управления бризерами `Tion 4S`, `Tion Lite` и `Tion 3S` с помощью ESP32 через BLE протокол и бризером `Tion 4S` посредсвом прямого подключения через интеграционный разъем с помощью стика ESP32/ESP8266 (как уточняйте в чате [Telegram][telegram]).

На текущий момент компонент построен на платформе `climate` и позволяет контролировать:

* Включение/Выключение
* Включение/Выключение обогрева
* Выставление целевой температуры нагрева
* Установка скорости притока воздуха
* Управление звуковыми оповещениями
* Управление световымы оповещениями (только для 4S и Lite)
* Переключение режима приток/рециркуляция (только для 4S)
* Переключение режима приток/рециркуляция/смешанный (только для 3S)
* Поддержка пресетов
* Настройка времени пресета `Турбо`

Дополнительно осуществляется мониториг следующих показателей:

* Температура снаружи
* Температура внутри
* Текущая потребяемая мощность нагревателя (только для 4S и Lite)
* Оставшееся время жизни фильтров
* Индикация о требущейся замене/очистке фильтра (только для 4S и Lite)
* Оставшееся время работы режима `Турбо`
* Счетчик прошедшего воздуха (только для 4S и Lite)
* Текущая производительностью бризера (только для 3S)
* Версия програмного обеспечения бризера

## Прошивка

Вы можете загрузить и использовать примеры конфигурации для [Tion 4S](tion-4s.yaml), [Tion Lite](tion-lt.yaml) и [Tion 3S](tion-3s.yaml), все файлы с подробным описанием (на английском)

* Скачайте конфигурацию соотствующую модели вашего бризера
* Измените секцию `substitutions` согласно вашим предпочтениям
* Измените набор подключаемых пакетов по вашему вкусу
* Поместите модифицированный файл в директорию с конфигурацией ESPHome
* Запустите сборку и прошивку вашей конфигурации
* Добавьте появившееся устройство в Home Assistant

## Использование
После [прошивки](#прошивка) и перед первым использование вам необъходимо ввести свой бризер в режим сопряжения (см. инструкцию) и только потом включать ESP.

Дополнительно, только для `Tion 3S`, необходимо нажать кнопку `Pair` в Home Assistant.

>
> ### **ВНИМАНИЕ: Все что вы делаете вы делаете только на свой страх и риск!**
>

## Планы на будущее

* Поддержка физического подключения через интеграционный разъем для `Tion 4S` (так же исследую возможность для 3S и Lite).
* Управление сбросом ресура фильтров
* Автоматическое управление притоком от внешнего датчика CO2

## Решение проблем и поддержка новых функций

Не стесняйтесь открывать [задачи](https://github.com/dentra/esphome-tion/issues) для сообщений об ошибках и запросов новых функций.

Так же вы можете воспользоваться [группой в Telegram](https://t.me/esphome_tion).

## Ваша благодарность

Если этот проект оказался для вас полезен и/или вы хотите поддержать его дальнейше развитие, то всегда можно оставить вашу благодарность
через [Card2Card](https://www.tinkoff.ru/cf/3dZPaLYDBAI) или [PayPal](https://paypal.me/dentra0).

## Коммерческое использование

По вопросам коммерческого использования или заказной разработки/доработки, обращийтесь по почте dennis.trachuk на gmail.com
