#include "ArduinoJson.h"
#include "PubSubClient.h"
#include "Ethernet.h"
#include "Ticker.h"
#include "ZHNetwork.h"
#include "ZHSmartHomeProtocol.h"
#include "ZHRF24SensorProtocol.h"

//***********************КОНФИГУРАЦИЯ***********************//
#define CS_PORT 5 // Измените (при необходимости) GPIO подключения cетевого модуля W5500 ТСР/IP.

const char *myNetName{"SMART"}; // Укажите имя сети ESP-NOW.

// Настройки сети:
byte mac[6] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED}; // Измените (при необходимости) MAC адрес cетевого модуля W5500 ТСР/IP.
IPAddress ip(192, 168, 2, 3);                       // Укажите статический IP (при необходимости).

// Настройки MQTT:
const char *mqttHostName{"192.168.2.2"};
const uint16_t mqttHostPort{1883};
const char *mqttUserID{"ID"};
const char *mqttUserLogin{""};
const char *mqttUserPassword{""};
const String topicPrefix{"homeassistant"};
//***********************************************************//

void onBroadcastReceiving(const char *data, const byte *sender);
void onUnicastReceiving(const char *data, const byte *sender);
void restart(void);
void attributesMessage(void);
void keepAliveMessage(void);
void onMqttMessage(char *topic, byte *payload, unsigned int length);
String getValue(String data, char separator, int index);

const String firmware{"1.0"};
uint64_t keepAliveMessageLastTime{0};
const uint16_t keepAliveMessageTimerDelay{10000};
uint64_t mqttConnectionCheckLastTime{0};
const uint16_t mqttConnectionCheckTimerDelay{5000};
bool semaphoreForKeepAliveMessage{true};

ZHNetwork myNet;
EthernetClient lanClient;
PubSubClient mqttClient(lanClient);

Ticker restartTimer;

void setup()
{
    Ethernet.init(CS_PORT);
    Ethernet.begin(mac, ip); // Статический IP.
    // Ethernet.begin(mac); // Динамический IP.
    myNet.begin(myNetName);
    myNet.setOnBroadcastReceivingCallback(onBroadcastReceiving);
    myNet.setOnUnicastReceivingCallback(onUnicastReceiving);
    mqttClient.setServer(mqttHostName, mqttHostPort);
    mqttClient.setCallback(onMqttMessage);
}

void loop()
{
    if (!mqttClient.connected() && (millis() - mqttConnectionCheckLastTime) > mqttConnectionCheckTimerDelay)
    {
        if (mqttClient.connect(mqttUserID, mqttUserLogin, mqttUserPassword))
        {
            attributesMessage();
            keepAliveMessage();
            mqttClient.subscribe(String(topicPrefix + "/gateway/#").c_str());
            mqttClient.subscribe(String(topicPrefix + "/espnow_switch/#").c_str());
            mqttClient.subscribe(String(topicPrefix + "/espnow_led/#").c_str());
        }
        mqttConnectionCheckLastTime = millis();
    }
    if ((millis() - keepAliveMessageLastTime) > keepAliveMessageTimerDelay && semaphoreForKeepAliveMessage)
    {
        keepAliveMessage();
        keepAliveMessageLastTime = millis();
    }
    mqttClient.loop();
    myNet.maintenance();
}

void onBroadcastReceiving(const char *data, const uint8_t *sender)
{
    PayloadsData incomingData;
    os_memcpy(&incomingData, data, sizeof(PayloadsData));
    if (incomingData.deviceType == SENSOR)
    {
        // В процессе. Будут добавлены датчик открытия/закрытия и датчик протечки воды.
    }
}

void onUnicastReceiving(const char *data, const uint8_t *sender)
{
    PayloadsData incomingData;
    os_memcpy(&incomingData, data, sizeof(PayloadsData));
    if (incomingData.payloadsType == ATTRIBUTES)
    {
        if (incomingData.deviceType == SWITCH)
            mqttClient.publish(String(topicPrefix + "/espnow_switch/" + myNet.macToString(sender) + "/attributes").c_str(), incomingData.message, true);
        if (incomingData.deviceType == LED)
            mqttClient.publish(String(topicPrefix + "/espnow_led/" + myNet.macToString(sender) + "/attributes").c_str(), incomingData.message, true);
        if (incomingData.deviceType == RF24_GATEWAY)
            mqttClient.publish(String(topicPrefix + "/gateway/" + myNet.macToString(sender) + "/attributes").c_str(), incomingData.message, true);
    }
    if (incomingData.payloadsType == KEEP_ALIVE)
    {
        if (incomingData.deviceType == SWITCH)
            mqttClient.publish(String(topicPrefix + "/espnow_switch/" + myNet.macToString(sender) + "/status").c_str(), "online", true);
        if (incomingData.deviceType == LED)
            mqttClient.publish(String(topicPrefix + "/espnow_led/" + myNet.macToString(sender) + "/status").c_str(), "online", true);
        if (incomingData.deviceType == RF24_GATEWAY)
            mqttClient.publish(String(topicPrefix + "/gateway/" + myNet.macToString(sender) + "/status").c_str(), "online", true);
    }
    if (incomingData.payloadsType == STATE)
    {
        if (incomingData.deviceType == SWITCH)
            mqttClient.publish(String(topicPrefix + "/espnow_switch/" + myNet.macToString(sender) + "/switch/state").c_str(), incomingData.message, true);
        if (incomingData.deviceType == LED)
            mqttClient.publish(String(topicPrefix + "/espnow_led/" + myNet.macToString(sender) + "/light/state").c_str(), incomingData.message, true);
        if (incomingData.deviceType == SENSOR)
            mqttClient.publish(String(topicPrefix + "/espnow_sensor/" + myNet.macToString(sender) + "/state").c_str(), incomingData.message, true);
        if (incomingData.deviceType == RF24_GATEWAY)
        {
            PayloadsData jsonData;
            os_memcpy(jsonData.message, incomingData.message, sizeof(incomingData.message));
            StaticJsonDocument<sizeof(jsonData.message)> json;
            deserializeJson(json, jsonData.message);
            if (int(json["type"]) == BME280)
                mqttClient.publish(String(topicPrefix + "/rf24_sensor/bme280/" + int(json["id"])).c_str(), incomingData.message, true);
            if (int(json["type"]) == BMP280)
                mqttClient.publish(String(topicPrefix + "/rf24_sensor/bmp280/" + int(json["id"])).c_str(), incomingData.message, true);
            if (int(json["type"]) == BME680)
                mqttClient.publish(String(topicPrefix + "/rf24_sensor/bme680/" + int(json["id"])).c_str(), incomingData.message, true);
            if (int(json["type"]) == TOUCH_SWITCH)
                mqttClient.publish(String(topicPrefix + "/rf24_sensor/touch_switch/" + int(json["id"])).c_str(), incomingData.message, false);
            if (int(json["type"]) == WATER_LEAKAGE)
                mqttClient.publish(String(topicPrefix + "/rf24_sensor/water_leakage/" + int(json["id"])).c_str(), incomingData.message, true);
            if (int(json["type"]) == PLANT_HUMIDITY)
                mqttClient.publish(String(topicPrefix + "/rf24_sensor/plant_humidity/" + int(json["id"])).c_str(), incomingData.message, true);
            if (int(json["type"]) == OPEN_CLOSE)
                mqttClient.publish(String(topicPrefix + "/rf24_sensor/open_close/" + int(json["id"])).c_str(), incomingData.message, true);
        }
    }
}

void restart()
{
    ESP.restart();
}

void attributesMessage()
{
    StaticJsonDocument<124> json;
    json["MCU"] = "ESP8266";
    json["MAC"] = myNet.getNodeMac();
    json["Firmware"] = firmware;
    json["Library"] = myNet.getFirmwareVersion();
    json["IP"] = Ethernet.localIP().toString();
    char buffer[124];
    serializeJsonPretty(json, buffer);
    mqttClient.publish(String(topicPrefix + "/gateway/" + myNet.getNodeMac() + "/attributes").c_str(), buffer, true);
}

void keepAliveMessage()
{
    mqttClient.publish(String(topicPrefix + "/gateway/" + myNet.getNodeMac() + "/status").c_str(), "online", true);
    PayloadsData outgoingData{GATEWAY, KEEP_ALIVE};
    char temp[sizeof(PayloadsData)];
    os_memcpy(temp, &outgoingData, sizeof(PayloadsData));
    myNet.sendBroadcastMessage(temp);
}

void onMqttMessage(char *topic, byte *payload, unsigned int length)
{
    String mac = getValue(String(topic).substring(0, sizeof(PayloadsData)), '/', 2);
    String message;
    bool flag{false};
    for (uint16_t i = 0; i < length; ++i)
    {
        message += (char)payload[i];
    }
    PayloadsData outgoingData;
    outgoingData.deviceType = GATEWAY;
    StaticJsonDocument<sizeof(outgoingData.message)> json;
    if (message == "update" || message == "restart")
    {
        flag = true;
        if (mac == myNet.getNodeMac())
        {

            semaphoreForKeepAliveMessage = false;
            mqttClient.publish(String(topicPrefix + "/gateway/" + mac).c_str(), "", true);
            mqttClient.publish(String(topicPrefix + "/gateway/" + mac + "/status").c_str(), "offline", true);
            if (message == "update")
            {
                mqttClient.disconnect();
                myNet.update();
                restartTimer.once(300, restart);
            }
            else
                restart();
            return;
        }
        String temp = String(topic);
        mqttClient.publish(String(temp).c_str(), "", true);
        mqttClient.publish(String(temp + "/status").c_str(), "offline", true);
    }
    if (String(topic) == topicPrefix + "/espnow_switch/" + mac + "/switch/set" || String(topic) == topicPrefix + "/espnow_led/" + mac + "/light/set")
    {
        flag = true;
        json["set"] = message == "ON" ? "ON" : "OFF";
    }
    if (String(topic) == topicPrefix + "/espnow_led/" + mac + "/light/brightness")
    {
        flag = true;
        json["brightness"] = message;
    }
    if (String(topic) == topicPrefix + "/espnow_led/" + mac + "/light/temperature")
    {
        flag = true;
        json["temperature"] = message;
    }
    if (String(topic) == topicPrefix + "/espnow_led/" + mac + "/light/rgb")
    {
        flag = true;
        json["rgb"] = message;
    }
    if (flag)
    {
        outgoingData.payloadsType = message == "update" ? UPDATE : SET;
        char buffer[sizeof(outgoingData.message)];
        serializeJsonPretty(json, buffer);
        os_memcpy(outgoingData.message, buffer, sizeof(outgoingData.message));
        char temp[sizeof(PayloadsData)];
        os_memcpy(temp, &outgoingData, sizeof(PayloadsData));
        byte target[6];
        myNet.stringToMac(mac, target);
        myNet.sendUnicastMessage(temp, target);
    }
}

String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = {0, -1};
    int maxIndex = data.length() - 1;
    for (int i = 0; i <= maxIndex && found <= index; i++)
    {
        if (data.charAt(i) == separator || i == maxIndex)
        {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i + 1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}