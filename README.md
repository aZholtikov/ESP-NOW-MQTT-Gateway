# Шлюз для устройств ESP-NOW для ESP8266 + W5500
Шлюз для обмена данными между устройствами ESP-NOW и MQTT брокером на основе ESP-NOW для ESP8266.

## Функции:

1. Никакого WiFi и сторонних серверов. Всё работает исключительно локально.
2. Периодическая передача своего состояния доступности (Keep Alive) в сеть ESP-NOW и на MQTT брокер.
  
## Примечание:

1. Работает на основе библиотеки [ZHNetwork](https://github.com/aZholtikov/ZHNetwork) и протоколов передачи данных [ZH Smart Home Protocol](https://github.com/aZholtikov/ZH-Smart-Home-Protocol) и [ZH RF24 Sensor Protocol](https://github.com/aZholtikov/ZH-RF24-Sensor-Protocol).
2. Для включения режима обновления прошивки необходимо послать команду "update" в корневой топик устройства (пример - "homeassistant/espnow_led/70039F44BEF7"). Устройство перейдет в режим обновления (подробности в [API](https://github.com/aZholtikov/ZHNetwork/blob/master/src/ZHNetwork.h) библиотеки [ZHNetwork](https://github.com/aZholtikov/ZHNetwork)). Аналогично для перезагрузки послать команду "restart".
3. При возникновении вопросов/пожеланий/замечаний пишите на github@zh.com.ru

## Поддерживаемые устройства:

1. [RF24 ESP-NOW Gateway](https://github.com/aZholtikov/RF24-ESP-NOW-Gateway)
2. [ESP-NOW Switch](https://github.com/aZholtikov/ESP-NOW-Switch)
3. [ESP-NOW LED Light/Strip](https://github.com/aZholtikov/ESP-NOW-LED-Light-Strip)

## Версии:

1. v1.0 Начальная версия.