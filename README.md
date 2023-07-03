[![Version][version-shield]][version]
[![License][license-shield]][license]
[![ESPHome release][esphome-release-shield]][esphome-release]
[![Telegram][telegram-shield]][telegram]
[![Support author][donate-tinkoff-shield]][donate-tinkoff]
[![Support author][donate-boosty-shield]][donate-boosty]
[![Open in Visual Studio Code][open-in-vscode-shield]][open-in-vscode]

[version-shield]: https://img.shields.io/static/v1?label=Версия&message=2023.7.0&color=green
[version]: https://github.com/dentra/esphome-tion/releases/

[license-shield]: https://img.shields.io/static/v1?label=Лицензия&message=MIT&color=orange&logo=license
[license]: https://opensource.org/licenses/MIT

[esphome-release-shield]: https://img.shields.io/static/v1?label=ESPHome&message=2023.6&color=green&logo=esphome
[esphome-release]: https://github.com/esphome/esphome/releases/

[open-in-vscode-shield]: https://img.shields.io/static/v1?label=+&message=Открыть+в+VSCode&color=blue&logo=visualstudiocode
[open-in-vscode]: https://open.vscode.dev/dentra/esphome-tion

[telegram-shield]: https://img.shields.io/static/v1?label=Поддержка&message=Телеграм&logo=telegram&color=blue
[telegram]: https://t.me/esphome_tion

[donate-tinkoff-shield]: https://img.shields.io/static/v1?label=Поддержать+автора&message=Тинькофф&color=yellow
[donate-tinkoff]: https://www.tinkoff.ru/cf/3dZPaLYDBAI

[donate-boosty-shield]: https://img.shields.io/static/v1?label=Поддержать+автора&message=Boosty&color=red
[donate-boosty]: https://boosty.to/dentra

# Tion

English version of this page is available via [google translate](https://github-com.translate.goog/dentra/esphome-tion?_x_tr_sl=ru&_x_tr_tl=en&_x_tr_hl=en&_x_tr_pto=wapp).

Компонет ESPHome для управления бризерами `Tion` с помощью ESP в вашей системе управления умным домом. Поддерживаются Home Asistant [API](https://esphome.io/components/api.html) и [MQTT](https://esphome.io/components/mqtt.html).

Поддерживаемые модели и протоколы:
 - Tion 4S (BLE/UART)
 - Tion Lite (BLE)
 - Tion 3S (BLE/UART)

Компонент построен на платформе [climate](https://esphome.io/components/climate/index.html) и позволяет контролировать:
* Включение/Выключение
* Включение/Выключение обогрева
* Выставление целевой температуры нагрева
* Установка скорости притока воздуха
* Управление звуковыми оповещениями
* Управление световымы оповещениями (только для 4S и Lite)
* Переключение режима приток/рециркуляция (только для 4S)
* Переключение режима приток/рециркуляция/смешанный (только для 3S)
* Поддержка пресетов
* Настройка времени пресета "Турбо"
* Конфигурация пресетов сервисом
* Сброс ресурса фильтров

Дополнительно осуществляется мониториг следующих показателей:
* Температура снаружи
* Температура внутри
* Текущая потребяемая мощность нагревателя (только для 4S и Lite)
* Оставшееся время жизни фильтров
* Индикация о требущейся замене/очистке фильтра (только для 4S и Lite)
* Оставшееся время работы режима "Турбо"
* Счетчик прошедшего воздуха (только для 4S и Lite)
* Текущая производительностью бризера (только для 3S)
* Версия програмного обеспечения бризера


>
> ### **ВНИМАНИЕ: Все что вы делаете вы делаете только на свой страх и риск!**
>

## Подключение

Доступно два вида подключения BLE и UART.

BLE подключение работает так же как ваш пульт или официальное приложение. UART подключние различно для разных моделей бризеров.

Для UART-подключения `Tion 4S` используется штатный интеграционный разъем бризера. Рекомендуется приобрести стик [Lilygo T-Dongle S3](https://github.com/Xinyuan-LilyGO/T-Dongle-S3), проще всего это сделать на Aliexpress. Или собрать самостоятельно на базе ESP32.

Для UART-подключения `Tion 3S` использется штатный, но доступный только при небольшой разборке бризера, разъем. Здесь рекумендуется использовать любую ESP8266 (ESP32 не тянет по питанию) используя [схему подключения](hardware/3s/NodeMCUv3-Tion.pdf). При использовании ESP-01S не будет доступено подключение штатного модуля BLE, в остальных случаях подключение будет полным.

По вопросам подключению велкам в чат [Telegram][telegram].

## Прошивка

Вы можете загрузить и использовать примеры конфигурации для [Tion 4S BLE](tion-4s.yaml), [Tion 4S UART](tion-4s-hw.yaml), [Tion Lite BLE](tion-lt.yaml), [Tion 3S BLE](tion-3s.yaml) и [Tion 3S UART](tion-3s-hw.yaml), все файлы с подробным описанием (на английском)

* Скачайте конфигурацию соотствующую модели вашего бризера
* Измените секцию `substitutions` согласно вашим предпочтениям
* Измените набор подключаемых пакетов по вашему вкусу
* Поместите модифицированный файл в директорию с конфигурацией ESPHome
* Запустите сборку и прошивку вашей конфигурации
* Добавьте появившееся устройство в Home Assistant

## Использование в режиме BLE
После [прошивки](#прошивка) и перед первым использование вам необъходимо ввести свой бризер в режим сопряжения (см. инструкцию) и только потом включать ESP.

Дополнительно, только для `Tion 3S`, необходимо нажать кнопку `Pair` в Home Assistant.

## Использование в режиме UART

Никаких дополнительный действий не требуется.

## Планы на будущее

* ~~Поддержка физического подключения через интеграционный разъем для `Tion 4S`~~ (так же исследую возможность для ~~3S и~~ Lite)
* ~~Управление сбросом ресура фильтров~~
* Автоматическое управление притоком от внешнего датчика CO2

## Решение проблем и поддержка новых функций

Не стесняйтесь открывать [задачи](https://github.com/dentra/esphome-tion/issues) для сообщений об ошибках и запросов новых функций.

Так же вы можете воспользоваться [группой в Telegram](https://t.me/esphome_tion).

## Ваша благодарность

Если этот проект оказался для вас полезен и/или вы хотите поддержать его дальнейше развитие, то всегда можно оставить вашу благодарность [переводом на карту](https://www.tinkoff.ru/cf/3dZPaLYDBAI) или [подпиской](https://boosty.to/dentra).

## Коммерческое использование

По вопросам коммерческого использования или заказной разработки/доработки, обращийтесь по почте dennis.trachuk на gmail.com
